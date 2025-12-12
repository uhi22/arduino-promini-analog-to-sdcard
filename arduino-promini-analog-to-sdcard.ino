/*
  Arduino writes analog values to SD card.

  Board: Arduino pro mini
  The circuit: SD card attached to SPI bus as follows:
    MOSI - pin 11
    MISO - pin 12
    CLK - pin 13
    CS - pin 4
  Analog signal on A0.

  Initial example taken from https://www.arduino.cc/en/Tutorial/LibraryExamples/ReadWrite

  When to close or flush the file?
    Discussed here: https://forum.arduino.cc/t/sd-card-do-i-need-to-close-after-each-write/614994
    Conclusions: Flush and close are quite expensive, they lead to writing at least 2024 bytes to the SD card.
    So we do not want this for each sample. But Flush or close are necessary to update the directory entry,
    otherwise the files will be shown with zero size when viewed on PC.
    So we use the strategy: After some seconds of recording, flush the data. In case of power-loss,
    the data after the last flush is lost. This is tolerable.

*/

#include <SPI.h>
#include <SD.h>

#define PIN_IN_LEADOFF_MINUS 3
#define PIN_IN_LEADOFF_PLUS 2
#define SD_CARD_CS_PIN 4
#define PIN_IN_BUTTON 5
#define PIN_BUZZER 6
#define PIN_OUT_LEDWHITE 9
#define PIN_OUT_LEDRED 8

#define FLUSH_CYCLE_MS 10000  /* after 10s, flush the write buffer to the SD card */
#define STORE_CYCLE_TIME_MS 5 /* each 5ms store the samples in the RAM buffer to the SD card. */
#define EXTENDED_RECORD_CYCLE_TIME_MS 1000 /* once per second we want to store a "long" data record with statistics */
#define NEW_FILE_CYCLE_TIME_MINUTES 10  /* limits the duration of one file. We use a new file name after this time. */
#define NEW_FILE_CYCLE_TIME_MS (NEW_FILE_CYCLE_TIME_MINUTES*60ul*1000ul)

/* we want 500Hz (2ms) sampling, and a buffer which covers
   at least 100ms worst case write time of the SD card. So 100ms/2ms = 50 Samples. */
#define ADC_FIFO_LENGTH 50
uint16_t adcFifoValue[ADC_FIFO_LENGTH];
uint32_t adcFifoTimestamp_ms[ADC_FIFO_LENGTH];
volatile uint16_t adcFifoWriteIndex, adcFifoReadIndex;
volatile uint16_t adcFifoOverrunCounter;

volatile uint32_t isrNumberOfAdcInterrupts;
volatile uint32_t isrNumberOf500usInterrupts;
volatile uint8_t isr500usDivider;
volatile uint16_t latestAdcSample;
volatile uint8_t blToneOn, blUseTone;
volatile uint8_t blPeakDetected;
volatile uint8_t blRecordingEnabled;

uint8_t nButtonDebounce, nButtonActuations;

File myFile;
volatile uint16_t hwstatus;
uint32_t time_ms;
uint32_t timeLastFlush_ms;
uint32_t timeLastStore_ms;
uint32_t timeExtendedRecord_ms;
uint32_t timeLastNewFile_ms;
uint8_t blTriggerExtendedRecord;
uint8_t blTriggerNewFile;
uint16_t filenumber;
uint32_t sampleNumber;

#define PEAK_SIZE_FOR_LED_FLASH 70
#define SHORT_TERM_HISTORY_LENGTH 25 /* e.g. 25 means 25 samples means 50ms */
uint16_t shortTermHistory[SHORT_TERM_HISTORY_LENGTH];
uint8_t shortTermHistoryWriteIndex;

void showUnrecoverableError(void) {
  /* This is reached in cases when something is generally broken. User needs to do a power-on-reset. */
  while (1) {
    digitalWrite(PIN_OUT_LEDRED, 1);
    digitalWrite(PIN_OUT_LEDWHITE, 0);
    blToneOn = 0;
    delay(100);
    digitalWrite(PIN_OUT_LEDRED, 0);
    digitalWrite(PIN_OUT_LEDWHITE, 1);
    blToneOn = 1;
    delay(100);
  }
}

void startRecording(void) {
  adcFifoWriteIndex = 0; /* start with an empty FIFO */
  adcFifoReadIndex = 0;
  blRecordingEnabled = 1;
}

void stopRecording(void) {
  blRecordingEnabled = 0;
}

void findUnusedFileNameAndOpenItForWriting(void) {
  /* tries different file names until an unused file name is found. */
  char strFileName[20];
  filenumber = 0;
  while (1) { /* loop until we find a free file name */
    sprintf(strFileName, "DAT%05d.txt", filenumber); /* create a file name "DAT00001.txt", with running number. */
    myFile = SD.open(strFileName); /* try to open the file for reading */
    if (myFile) {
      //Serial.print(strFileName);
      //Serial.println(F(" exists"));
      myFile.close();
      filenumber++; /* try the next file name */
    } else {
      /* if the file didn't open, it does not exist. This means, we found an unused file name. */
      //Serial.print(strFileName);
      //Serial.println(F(" does not exist"));
      myFile = SD.open(strFileName, FILE_WRITE); /* try to create the file */
      /* if the file opened okay, write to it: */
      if (myFile) {
        Serial.print(String(F("New file ")) + String(strFileName));
        myFile.println(F("time_ms,adc,status,overruns,buttons"));
      } else {
        /* if the file didn't open, print an error */
        Serial.println(F("error opening file for write"));
        showUnrecoverableError();
      }
      break;
    }
  } 
}


void calculateHardwareStatusAndShowOnLEDs(void) {
    hwstatus = 0;
    if (digitalRead(PIN_IN_LEADOFF_MINUS)) { hwstatus |= 1; } /* lead minus is off */
    if (digitalRead(PIN_IN_LEADOFF_PLUS)) { hwstatus |= 2; } /* lead plus is off */
    if (hwstatus!=0) {
      digitalWrite(PIN_OUT_LEDRED, 1); /* any lead off -> red LED on */
    } else {
      digitalWrite(PIN_OUT_LEDRED, 0); /* everything fine -> red LED off */
    }
    if (latestAdcSample<50) hwstatus |= 4; /* voltage is very low */
    if (latestAdcSample>950) hwstatus |= 8; /* voltage is very high */
}

void evaluateShortTermHistoryAndControlActivityLED(void) {
  int8_t indexOldest, indexLatest, indexMiddle;
  shortTermHistory[shortTermHistoryWriteIndex] = latestAdcSample;
  shortTermHistoryWriteIndex++;
  if (shortTermHistoryWriteIndex>=SHORT_TERM_HISTORY_LENGTH) shortTermHistoryWriteIndex=0;
  indexOldest = shortTermHistoryWriteIndex; /* the write index points to the data which will be written next, so this is the oldest. */
  indexLatest = indexOldest-1; if (indexLatest<0) indexLatest+=SHORT_TERM_HISTORY_LENGTH;
  indexMiddle = indexOldest + SHORT_TERM_HISTORY_LENGTH/2; if (indexMiddle>=SHORT_TERM_HISTORY_LENGTH) indexMiddle-=SHORT_TERM_HISTORY_LENGTH;
  if ((shortTermHistory[indexMiddle]>shortTermHistory[indexOldest]+PEAK_SIZE_FOR_LED_FLASH) &&
      (shortTermHistory[indexMiddle]>shortTermHistory[indexLatest]+PEAK_SIZE_FOR_LED_FLASH)) {
        digitalWrite(PIN_OUT_LEDWHITE, 1); /* if we see a peak in the middle, light the white LED */
        blPeakDetected = 1;
      } else {
        digitalWrite(PIN_OUT_LEDWHITE, 0);
        blPeakDetected = 0;
      }
}

void controlTheTone(void) {
  if (hwstatus != 0) {
    blToneOn = 1; /* create error tone */
  } else if (blPeakDetected) {
    blToneOn = 1; /* the normal activity tone */
  } else {
    blToneOn = 0; /* silence */
  }
}

void storeCollectedSamplesIntoFile(void) {
  char strTmp[30];

  uint32_t t_ms;
  uint16_t y;

  while (adcFifoReadIndex != adcFifoWriteIndex) {
    t_ms = adcFifoTimestamp_ms[adcFifoReadIndex];
    y = adcFifoValue[adcFifoReadIndex];
    adcFifoReadIndex++;
    if (adcFifoReadIndex>=ADC_FIFO_LENGTH) adcFifoReadIndex = 0;

    if (blTriggerExtendedRecord) {
      /* log the long entry including statistics */
      sprintf(strTmp, "%ld,%d,%d,%d,%d", t_ms, y, hwstatus, adcFifoOverrunCounter,nButtonActuations);
      blTriggerExtendedRecord = 0; /* consume the trigger */
    } else {
      /* normally only log the timestamp and the analog input */
      sprintf(strTmp, "%ld,%d", t_ms, y);
    }
    //Serial.println(String(time_ms)+"\t"+String(u));
    if (myFile) { myFile.println(strTmp); }
    sampleNumber++;


    if (blTriggerNewFile) {
      blTriggerNewFile = 0;
      /* quater of an hour of samples. Choose a new file name. */
      if (myFile) {
        stopRecording(); /* disable sampling while we are creating a new file */
        myFile.close();
        findUnusedFileNameAndOpenItForWriting();
        startRecording(); /* new file is ready, continue sampling */
      } else {
        /* no file is open. This is a severe error. */
        showUnrecoverableError();
      }
    }
  }
}


void printStatistics(void) {
  Serial.print(F("500usInt "));
  Serial.print(isrNumberOf500usInterrupts);
  Serial.print(F(", nAdcInt "));
  Serial.print(isrNumberOfAdcInterrupts);
  Serial.print(F(", Overrun "));
  Serial.print(adcFifoOverrunCounter);
  Serial.print(F(", hwstat "));
  Serial.print(hwstatus);
  Serial.print(F(", btns"));
  Serial.println(nButtonActuations);
}

void evaluateButton(void) {
  if (digitalRead(PIN_IN_BUTTON)==0) {
    /* if button is pressed (low active) */
    nButtonDebounce++;
    if (nButtonDebounce==3) {
      nButtonActuations++;
    }
  } else {
    /* button not pressed --> reset the debounce counter */
    nButtonDebounce = 0;
  }
}

/* Timer1 Compare A interrupt - triggers every 500µs */
ISR(TIMER1_COMPA_vect) {
  isr500usDivider=(isr500usDivider+1) % 4; /* counting from 0 to 3. To create a 2ms cycle. */
  if (isr500usDivider==0) {
    if (blRecordingEnabled) {
      /* in 2ms cycle, we want to sample the analog input */
      ADCSRA |= (1 << ADSC); /* trigger the AD conversion */
    }
  }
  if (isr500usDivider & 1) { /* mirror the bit 0 to the buzzer, to create a 1kHz tone. But only if the tone is requested. */
    if (blUseTone && blToneOn) digitalWrite(PIN_BUZZER, 1);
  } else {
    digitalWrite(PIN_BUZZER, 0);
  }
  isrNumberOf500usInterrupts++;
}

/* ADC Conversion Complete interrupt */
ISR(ADC_vect) {  
  latestAdcSample = ADC; /* Read 10-bit result (0-1023) */
  adcFifoValue[adcFifoWriteIndex]=latestAdcSample;
  adcFifoTimestamp_ms[adcFifoWriteIndex]=isrNumberOf500usInterrupts/2; 
  adcFifoWriteIndex++;
  if (adcFifoWriteIndex >= ADC_FIFO_LENGTH) {
    adcFifoWriteIndex=0;
  }
  if (adcFifoWriteIndex == adcFifoReadIndex) {
    /* this indicates a buffer overflow. Strategy: we discard the complete buffer, and count the error. */
    adcFifoOverrunCounter++;
  }
  calculateHardwareStatusAndShowOnLEDs();
  evaluateShortTermHistoryAndControlActivityLED(); /* blink LED depending on the waveform */
  controlTheTone();
  isrNumberOfAdcInterrupts++;
}


void initializeTimerForAdcAndAdc(void) {
   // Disable interrupts during setup
  cli();
  
  // === Configure Timer1 for 500µs (2kHz) ===
  TCCR1A = 0;  // Normal mode
  TCCR1B = 0;
  TCNT1 = 0;   // Reset counter
  
  // CTC mode (Clear Timer on Compare Match)
  TCCR1B |= (1 << WGM12);
  
  // Prescaler = 8 (for 16MHz: 16MHz/8 = 2MHz timer clock)
  TCCR1B |= (1 << CS11);
  
  // Compare value: 2MHz / 2kHz = 1000 cycles for 500us
  OCR1A = 999;  // (count from 0 to 999 = 1000 cycles)
  
  // Enable Timer1 Compare A interrupt
  TIMSK1 |= (1 << OCIE1A);
  
  // === Configure ADC ===
  ADMUX = (1 << REFS0);  // AVcc reference, ADC0 (A0)
  
  ADCSRA = (1 << ADEN)   // Enable ADC
         | (1 << ADIE)   // Enable ADC interrupt
         | (1 << ADPS2)  // Prescaler = 128
         | (1 << ADPS1)
         | (1 << ADPS0);
  
  // Enable interrupts
  sei();
  
  //Serial.println(F("Timer-triggered ADC started"));
}

void setup() {
  pinMode(PIN_IN_LEADOFF_MINUS, INPUT);
  pinMode(PIN_IN_LEADOFF_PLUS, INPUT);
  pinMode(PIN_IN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_OUT_LEDWHITE, OUTPUT);
  pinMode(PIN_OUT_LEDRED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_OUT_LEDWHITE, 1);
  digitalWrite(PIN_OUT_LEDRED, 0);
  delay(300);
  digitalWrite(PIN_OUT_LEDWHITE, 0);
  digitalWrite(PIN_OUT_LEDRED, 1);
  delay(300);
  digitalWrite(PIN_OUT_LEDWHITE, 0);
  digitalWrite(PIN_OUT_LEDRED, 0);
  digitalWrite(PIN_BUZZER, 0);
  blUseTone = 1;
  Serial.begin(115200);
  Serial.print(F("Init SD card..."));
  if (!SD.begin(SD_CARD_CS_PIN)) {
    Serial.println(F("init failed!"));
    showUnrecoverableError();
  }
  Serial.println(F("done."));
  //printContentOfExistingFile();
  findUnusedFileNameAndOpenItForWriting();
  Serial.print(F("Init ADC and Timer"));
  initializeTimerForAdcAndAdc();
  startRecording();
}


void loop() {
  time_ms = millis(); /* the millis() rolls-over after 49 days. This should be sufficient, no extra handling. */
  if (time_ms-timeLastStore_ms>=STORE_CYCLE_TIME_MS) {
    storeCollectedSamplesIntoFile();
    evaluateButton();
    timeLastStore_ms = time_ms;
  }
  if (time_ms-timeLastFlush_ms>FLUSH_CYCLE_MS) {
    myFile.flush();
    timeLastFlush_ms = time_ms;
    //Serial.println("Flushed");
  }
  if (time_ms-timeExtendedRecord_ms>EXTENDED_RECORD_CYCLE_TIME_MS) {
    timeExtendedRecord_ms = time_ms;
    blTriggerExtendedRecord = 1;
    printStatistics();
  }
  if (time_ms-timeLastNewFile_ms>NEW_FILE_CYCLE_TIME_MS) {
    timeLastNewFile_ms = time_ms;
    blTriggerNewFile = 1;
  }
}



