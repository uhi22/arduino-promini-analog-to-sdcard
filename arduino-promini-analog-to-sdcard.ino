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

#define FLUSH_CYCLE_MS 10000  /* after 10s, flush the write buffer to the SD card */
#define ANALOG_SAMPLE_TIME_MS 1 /* configuration hint: use at least 1ms */



File myFile;
uint16_t u, status;
uint32_t time_ms;
uint32_t timeLastFlush_ms;
uint32_t timeLastSample_ms;
uint16_t filenumber;
uint16_t freeFilenumber;

void printContentOfExistingFile(void) {
  #define FILE_NAME "analog.txt"
  myFile = SD.open(FILE_NAME); /* open the file for reading */
  if (myFile) {
    Serial.println(String("content of ") + String(FILE_NAME));
    while (myFile.available()) { /* read from the file until there's nothing else in it */
      Serial.write(myFile.read());
    }
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println(String("error opening ") +  String(FILE_NAME));
  }
}

void writeSomethingToFileWithOpenAndClose(void) {
  myFile = SD.open(FILE_NAME, FILE_WRITE);
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print(String("Writing to ") + String(FILE_NAME));
    myFile.println("test 1 2 3");
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
}

void openFileForWriting(void) {
  char strFileName[20];
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  sprintf(strFileName, "dat%05d.txt", freeFilenumber);
  myFile = SD.open(strFileName, FILE_WRITE);
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print(String("Writing to ") + String(strFileName));
    myFile.println("time_s,adc,status");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
}

void findUnusedFileName(void) {
  /* tries different file names until an unused file name is found. */
  char strTmp[20];
  filenumber = 0;
  freeFilenumber=0xffff;
  while (freeFilenumber==0xffff) {
    sprintf(strTmp, "dat%05d.txt", filenumber); /* create a file name "dat000001.txt", with running number. */
    Serial.print(strTmp);
    myFile = SD.open(strTmp); /* try to open the file for reading */
    if (myFile) {
      Serial.println(" exists");
      myFile.close();
      filenumber++; /* try the next file name */
    } else {
      /* if the file didn't open, it does not exist. This means, we found an unused file name. */
      Serial.println(" does not exist");
      if (freeFilenumber==0xffff) {
        freeFilenumber = filenumber;
      }
    }
  }
}


void storeSampleIntoFile(void) {
  char strTmp[30];
  char floatStr[10]; // float converted to string
  float fTime_s;

    u = analogRead(A0);
    status = 0;
    if (digitalRead(PIN_IN_LEADOFF_MINUS)) { status |= 1; } /* lead minus is off */
    if (digitalRead(PIN_IN_LEADOFF_PLUS)) { status |= 2; } /* lead plus is off */
    if (u<50) status |= 4; /* voltage is very low */
    if (u>950) status |= 8; /* voltage is very high */
    fTime_s = (float)time_ms/1000;
    dtostrf(fTime_s, 8, 3, floatStr);  // 8 char min total width, 3 after decimal
    sprintf(strTmp, "%s,%d,%d", floatStr, u, status);
    //Serial.println(String(time_ms)+"\t"+String(u));
    if (myFile) { myFile.println(strTmp); }
}


void setup() {
  pinMode(PIN_IN_LEADOFF_MINUS, INPUT);
  pinMode(PIN_IN_LEADOFF_PLUS, INPUT);
  Serial.begin(115200);
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CARD_CS_PIN)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  //printContentOfExistingFile();
  findUnusedFileName();
  if (freeFilenumber!=0xFFFF) {
    openFileForWriting();
  }
}


void loop() {
  time_ms = millis();
  if (time_ms-timeLastSample_ms>=ANALOG_SAMPLE_TIME_MS) {
    storeSampleIntoFile();
    timeLastSample_ms = time_ms;
  }
  if (time_ms-timeLastFlush_ms>FLUSH_CYCLE_MS) {
    myFile.flush();
    timeLastFlush_ms = time_ms;
    //Serial.println("Flushed");
  }
}



