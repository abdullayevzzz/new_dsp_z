import serial
import serial.tools.list_ports
import keyboard
from collections import deque
from matplotlib import pyplot as plt
from matplotlib.widgets import Button

fulLen = 500 #default list length

plt.figure(figsize=(8,6), dpi=100)
axA = plt.axes([0.1, 0.80, 0.8, 0.15])
axA.plot(1)

axB = plt.axes([0.1, 0.55, 0.8, 0.15])
axB.plot(1)

axC = plt.axes([0.1, 0.30, 0.8, 0.15])
axC.plot(1)

print ("Serial interface is open")
#Automatic port finder
port_name = ''
ports = list(serial.tools.list_ports.comports())
for p in ports:
    if 'XDS100' in p[1]:
        print("Device found")
        port_name = p[0]
        global ser
        ser = serial.Serial(port_name, 460800, timeout=1)
        break
        
if not ser:
    print ("Defice not found")
    exit()
ser.flush()

i = 0
buffer = ser.read(80)
while len(buffer) != 80:
    print('Buffer not read')
    buffer = ser.read(80)
while not (buffer[i] == 0x7F and buffer[(i+1, 0)[i+1 == 8]] == 0xFF):
    i = i+1    #find syncronization
ser.read(i) #throw away until next syncronisation
#now ser.read is synchronised


IR_list = deque(maxlen=fulLen)
RED_list = deque(maxlen=fulLen)
GREEN_list = deque(maxlen=fulLen)

i = 0
x_axis = range(0, fulLen * 5, 5)
while not keyboard.is_pressed("s"):
    buffer = ser.read(8)
    if (buffer[0] == 0x7F and buffer[1] == 0xFF):   #confirm synchronization
        RED_list.append(int.from_bytes(buffer[2:4], byteorder='little', signed = "False"))
        IR_list.append(int.from_bytes(buffer[4:6], byteorder='little', signed = "False"))
        GREEN_list.append(int.from_bytes(buffer[6:8], byteorder='little', signed = "False"))

        i += 1
        if i % 20 == 0 and len(RED_list) == fulLen:
            axA.cla()
            axA.set_title('RED', fontweight ="bold")
            axA.plot(x_axis, RED_list)

            axB.cla()
            axB.set_title('IR', fontweight ="bold")
            axB.plot(x_axis, IR_list)

            axC.cla()
            axC.set_title('GREEN', fontweight ="bold")
            axC.plot(x_axis, GREEN_list)

            plt.xlabel('time(ms)')
            plt.draw()
            plt.pause(0.01)

 
    
input('Press enter to exit')
