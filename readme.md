# Recording analog data and storing to SD card

## Measurements of "simple approach"

Precondition: AD conversion and writing to SD card are in the same task.

Result: Even if the average sample interval is near to the intended 1ms, there are bigger gaps
cause by the blocking nature of the SD card write access.

```
min sample time [s] 0.0009999999999763531
max sample time [s] 0.05000000000001137
avg sample time [s] 0.0011597428387528127
recorded time   [s] 929.7379999999999
number of samples longer than 2ms 35230
number of samples longer than 3ms 20813
number of samples longer than 5ms 7703
number of samples longer than 10ms 1780
```

## Next evolution step:

* use interrupt-based AD conversion, to get stable 1ms sample time
