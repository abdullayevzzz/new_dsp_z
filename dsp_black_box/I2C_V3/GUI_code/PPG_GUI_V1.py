refRes = complex(1000.0, 0.0) #reference resistor OHM
import serial
import keyboard
import cmath
import math
import csv
import struct
from matplotlib import pyplot as plt
from matplotlib.widgets import Button

fulLen = 800 #default list length

plt.figure(figsize=(8,4), dpi=100)
axA = plt.axes([0.1, 0.1, 0.8, 0.4])
axA.plot(1)

print ("Serial interface is open\n")
f = open('log.csv', 'w+', newline='')
writer = csv.writer(f)

ser = serial.Serial('COM6', 230400, timeout=1)
ser.flush()
#readByte = ser.read(1)
buffer = ser.read(500)
i = 0
while not (buffer[i] == 0x7F and buffer[(i+1, 0)[i+1 == 5]] == 0xFF):
    i = i+1    #find syncronization
ser.read(i) #throw away until next syncronisation
#now ser.read is synchronised
PPG_list = [0] * fulLen
i = 0
j = 0

header1 = ['PPG']

print(header1)
writer.writerow(header1)

while not keyboard.is_pressed("s"):
    buffer = ser.read(5)
    if (buffer[0] == 0x7F and buffer[1] == 0xFF):   #confirm synchronization
        PPG = int.from_bytes(buffer[2:5], byteorder='little', signed = "False")            
        PPG_list[j] = PPG        
        writer.writerow(PPG_list[j-1:j])
        j = j+1                   
        
       
        if (j==(fulLen)):
            axA.cla()
            axA.plot(PPG_list)
            axA.set_title('PPG', fontweight ="bold")  
            plt.draw()
            plt.pause(0.01)
            j = 0  
            #ax.cla()    
    #i = 0
    #while not (buffer[i] == 0x7F and buffer[(i+1, 0)[i+1 == 5]] == 0xFF): #modify code, make unique sync. code
    #    i = i+1    #find syncronization
    #ser.read(i) #throw away until next syncronisation
    #if (i > 0 and j > 0):  #synconization issue
    #    j = j-1
 
f.close()      
input('Press enter to exit')
