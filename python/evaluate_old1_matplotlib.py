import matplotlib.pyplot as plt
import numpy as np

def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False


# x = np.linspace(0, 2 * np.pi, 200)
# y = np.sin(x)

x=[]
y=[]

y2=[]
y3=[]


nLines=0
nSamples=0
tFloat=0
avgValue=[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
avgCount=[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]

#avg[2]=avg[2]+1
#for i in range(0, len(avg)):
#    print(avg[i])

# Open the file in read mode
with open('DAT00017.TXT', 'r') as file:
    # Read each line in the file
    for line in file:
        nLines=nLines+1
        # Print each line
        # print(line.strip())
        lineElements = line.split(",")
        if len(lineElements)==3:
            t = lineElements[0].strip()
            adc = lineElements[1].strip()
            if (is_number(t) and is_number(adc)):
                tFloat = float(t)
                adcFloat = float(adc)
                if (tFloat>115.3):
                    #adcFloat = 100-1*np.sin((tFloat+0.010)/0.02*2*3.1416)
                    #if (tFloat<115.2):
                    #    adcFloat=adcFloat+0
                    avgIndex = int(tFloat*1000) % 20
                    #print("index " + str(avgIndex) + ", old: " + str(avg[avgIndex]))
                    avgValue[avgIndex]+= adcFloat
                    avgCount[avgIndex]+=1
                    
                    x.append(tFloat)
                    y.append(adcFloat)
                    #sinValue = np.sin(tFloat/0.02*2*np.pi)
                    #cosValue = np.cos(tFloat/0.02*2*np.pi)
                    #y2.append(sinValue)
                    nSamples+=1
        if tFloat>115.5:
            break

avgOverAverages=0
for i in range(0, len(avgValue)):
    if (avgCount[i]>0):
        avgValue[i] = avgValue[i] / avgCount[i]
    avgOverAverages+=avgValue[i]
    # print(avgValue[i])
avgOverAverages/=len(avgValue) # The average over the whole

compensationValues = []
for i in range(0, len(avgValue)):
    compensationValues.append(avgValue[i] - avgOverAverages)

for i in range(0, len(y)):
    avgIndex = int(x[i]*1000) % 20
    compensation = compensationValues[avgIndex]
    y2.append(compensation)
    y3value = y[i] - compensation
    y3.append(y3value)

ax1 = plt.subplot(411)
plt.plot(x, y)
plt.tick_params('x', labelsize=6)

# share x only
ax2 = plt.subplot(412, sharex=ax1)
plt.plot(x, y2)

ax3 = plt.subplot(413, sharex=ax1)
plt.plot(x, y3)

ax4 = plt.subplot(414)
plt.plot(compensationValues, '-ob') # - means solid line, o means circle, b means blue

plt.show()


