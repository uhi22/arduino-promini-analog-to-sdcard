
def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False


x=[]
y=[]

y2=[]
y3=[]


nLines=0
nSamples=0
tFloat=0
tLastSample = -1
tDiff = -1
tDiffMax = 0
tDiffMin = 1000000
tDiffSum = 0
nSamplesForSum = 0
nSampleTimeAbove2ms = 0
nSampleTimeAbove3ms = 0
nSampleTimeAbove5ms = 0
nSampleTimeAbove10ms = 0

# Open the file in read mode
with open('A00007.TXT', 'r') as file:
    # Read each line in the file
    for line in file:
        nLines=nLines+1
        # Print each line
        # print(line.strip())
        lineElements = line.split(",")
        if len(lineElements)>=2:
            t = lineElements[0].strip()
            adc = lineElements[1].strip()
            if (is_number(t) and is_number(adc)):
                tFloat = float(t) * 0.001 # from ms into s
                adcFloat = float(adc)
                if (tFloat>0.2):
                    x.append(tFloat)
                    y.append(adcFloat)
                    if (tLastSample>0):
                        # we have a valid last sample time. Let's calcultate the sampling cycle time.
                        tDiff = tFloat - tLastSample
                        if (tDiff>tDiffMax):
                            tDiffMax = tDiff
                        if (tDiff<tDiffMin):
                            tDiffMin = tDiff
                        tDiffSum += tDiff
                        nSamplesForSum += 1
                        if (tDiff>0.002): # count, how many samples took longer than 2ms
                            nSampleTimeAbove2ms += 1
                        if (tDiff>0.003): # count, how many samples took longer than 3ms
                            nSampleTimeAbove3ms += 1
                        if (tDiff>0.005): # count, how many samples took longer than 5ms
                            nSampleTimeAbove5ms += 1
                        if (tDiff>0.010): # count, how many samples took longer than 10ms
                            nSampleTimeAbove10ms += 1
                            print("very long sampling time at " + str(tFloat))
                    tLastSample = tFloat
                    #sinValue = np.sin(tFloat/0.02*2*np.pi)
                    #cosValue = np.cos(tFloat/0.02*2*np.pi)
                    #y2.append(sinValue)
                    nSamples+=1
        #if tFloat>115.5:
        #    break

print("Statistics")
print("min sample time [s] " + str(tDiffMin))
print("max sample time [s] " + str(tDiffMax))
print("avg sample time [s] " + str(tDiffSum/nSamplesForSum))
print("recorded time   [s] " + str(tDiffSum))
print("number of samples longer than 2ms " + str(nSampleTimeAbove2ms))
print("number of samples longer than 3ms " + str(nSampleTimeAbove3ms))
print("number of samples longer than 5ms " + str(nSampleTimeAbove5ms))
print("number of samples longer than 10ms " + str(nSampleTimeAbove10ms))



