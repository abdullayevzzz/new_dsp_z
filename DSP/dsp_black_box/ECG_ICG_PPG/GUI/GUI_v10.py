refRes = complex(1000.0, 0.0) #reference resistor OHM
from scipy import signal
import serial
import serial.tools.list_ports
import keyboard
import cmath
import math
import csv
import struct
from matplotlib import pyplot as plt
from matplotlib.widgets import Button
from matplotlib.widgets import CheckButtons

fulLen = 500 #default mags_aver list length
halfLen = fulLen/2

DC = 0  #default dc value
DC_len = 50 #Length of hysteresis to calculate dc. Must be smaller than fulLen/4
AC_flag = False

filter_50_mode = 1
filter_100_mode = 1

mode_button = 'e' #default mode
freq_button = 3 #(20*10^freq)Hz default frequency

logFlag = 0

plt.figure(figsize=(8,6), dpi=100)
axA = plt.axes([0.1, 0.80, 0.8, 0.15])
axA.plot(1)

axB = plt.axes([0.1, 0.55, 0.8, 0.15])
axB.plot(1)

axC = plt.axes([0.1, 0.30, 0.8, 0.15])
axC.plot(1)

#Design notch filters
fs = 200.0  # Sample frequency (Hz)
f1 = 50.0  # Frequency to be removed from signal (Hz)
f2 = 99.99  # Frequency to be removed from signal (Hz)
Q = 30.0  # Quality factor
b50, a50 = signal.iirnotch(f1, Q, fs)
b100, a100 = signal.iirnotch(f2, Q, fs)


def filters(labels):
    global filter_50_mode
    global filter_100_mode    
    if (labels == '50Hz'):
        filter_50_mode = ~filter_50_mode
    if (labels == '100Hz'):
        filter_100_mode = ~filter_100_mode


def mode_d(event):
    global mode_button
    mode_button = 'd'
        
def mode_e(event):
    global mode_button
    mode_button = 'e'
    
def freq_1(event):
    global freq_button
    freq_button = 1
    
def freq_2(event):
    global freq_button
    freq_button = 2
    
def freq_3(event):
    global freq_button
    freq_button = 3
    
def stop(event):
    quit()
    
def log(event):
    global logFlag
    logFlag = 1
    
def DC_AC(event):
    pass
    #global AC_flag
    #AC_flag =  not AC_flag
    
ax1 = plt.axes([0.1, 0.15, 0.1, 0.1])
b1 = Button(ax1, '200Hz',color="yellow")
b1.on_clicked(freq_1)

ax2 = plt.axes([0.25, 0.15, 0.1, 0.1])
b2 = Button(ax2, '2kHz',color="yellow")
b2.on_clicked(freq_2)

ax3 = plt.axes([0.40, 0.15, 0.1, 0.1])
b3 = Button(ax3, '20kHz',color="yellow")
b3.on_clicked(freq_3)

ax7 = plt.axes([0.55, 0.15, 0.1, 0.1])
labels = ['50Hz','100Hz']
b7 = CheckButtons(ax7, labels, (True, True))
b7.on_clicked(filters)

ax4 = plt.axes([0.1, 0.02, 0.1, 0.1])
b4 = Button(ax4, 'Decimal',color="yellow")
b4.on_clicked(mode_d)

ax5 = plt.axes([0.25, 0.02, 0.1, 0.1])
b5 = Button(ax5, 'Floating',color="yellow")
b5.on_clicked(mode_e)

ax6 = plt.axes([0.40, 0.02, 0.1, 0.1])
b6 = Button(ax6, 'DC/AC',color="yellow")
b6.on_clicked(DC_AC)

ax8 = plt.axes([0.70, 0.02, 0.1, 0.1])
b8 = Button(ax8, 'Stop',color="yellow")
b8.on_clicked(stop)

ax9 = plt.axes([0.55, 0.02, 0.1, 0.1])
b9 = Button(ax9, 'Log',color="yellow")
b9.on_clicked(log)


print ("Serial interface is open")
f = open('log.csv', 'w+', newline='')
writer = csv.writer(f)


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
buffer = ser.read(14)
i = 0
while not (buffer[i] == 0x7F and buffer[(i+1, 0)[i+1 == 14]] == 0xFF):
    i = i+1    #find syncronization
ser.read(i) #throw away until next syncronisation
#now ser.read is synchronised
i = 0

ICG = [0] * fulLen
ICG_filt = [0] * fulLen
ECG = [0] * fulLen
ECG_filt = [0] * fulLen
PPG = [0] * fulLen
j = 0


freq = freq_button #(20*10^freq)kHz default frequency
ser.write(b'3')
mes = ['Default excitation signal frequency is ' + str(0.02*(10**(freq))) + 'kHz']
print(mes)
ser.read(1400)

mode = 'e' #default mode
ser.write(b'e')
ser.read(1400)

header1 = ['ICG', 'ICG_filt', 'ECG', 'ECG_Filtered', 'PPG']
print(header1)

while not keyboard.is_pressed("s"):
    buffer = ser.read(14)
    if (buffer[0] == 0x7F and buffer[1] == 0xFF):   #confirm synchronization
        if mode == 'd':
            #packet_number = int.from_bytes(buffer[2:4], byteorder='little')
            accumA1I = int.from_bytes(buffer[4:6], byteorder='little', signed = "True")
            accumA1Q = int.from_bytes(buffer[6:8], byteorder='little', signed = "True")
            accumB1I = int.from_bytes(buffer[8:10], byteorder='little', signed = "True")
            accumB1Q = int.from_bytes(buffer[10:12], byteorder='little', signed = "True")
            accumC = int.from_bytes(buffer[12:14], byteorder='little', signed = "False")
            PPG_sample = int.from_bytes(buffer[2:4], byteorder='little', signed = "False")
            accumA1 = complex (accumA1I,accumA1Q) 
            accumB1 = complex (accumB1I,accumB1Q)
            if (accumA1 == 0):
                accumA1 = accumA1+1
            Z1 = accumB1*refRes/accumA1
            magZ1 = abs(Z1)
            phZ1 = cmath.phase(Z1)
            
            ICG[j] = magZ1
            ECG[j] = accumC
            PPG[j] = PPG_sample
            
            #DC = sum(ICG)/len(ICG)
            if (AC_flag):
                pass
                #ICG[j] = mags_aver[j] - DC
            j = j+1
            if (j==fulLen):
                j = 0
                ECG_filt = ECG.copy()
                ICG_filt = ICG.copy()
                if (filter_50_mode):
                    #ECG_aver = signal.filtfilt(b50, a50, ECG_aver) #50Hz remove
                    for m in range ((fulLen)-4):
                        ECG_filt[m]  = (ECG[m] + ECG[m+1] + ECG[m+2] + ECG[m+3])/4
                        ICG_filt[m]  = (ICG[m] + ICG[m+1] + ICG[m+2] + ICG[m+3])/4
                        #leave end case
                if (filter_100_mode):
                    for m in range ((fulLen)-2):
                        ECG_filt[m]  = (ECG[m] + ECG[m+1])/2
                        ICG_filt[m]  = (ICG[m] + ICG[m+1])/2
                        #leave end case
                
                if (logFlag):
                    for r in range (fulLen):
                        data2 = ["%.4f" % ICG[r],"%.4f" % ICG_filt[r],"%.4f" % ECG[r],
                        "%.4f" % ECG_filt[r], "%.4f" % PPG[r]]
                        writer.writerow(data2)
            
        elif mode == 'e':
            #packet_number = int.from_bytes(buffer[2:4], byteorder='little')
            
            ratioReal = struct.unpack('<f', buffer[4:8])
            ratioImag = struct.unpack('<f', buffer[8:12])
            accumC = int.from_bytes(buffer[12:14], byteorder='little', signed = "False")
            PPG_sample = int.from_bytes(buffer[2:4], byteorder='little', signed = "False")
            
            ratio = complex(ratioReal[0],ratioImag[0])
            #Z1 = refRes/(ratio-1)
            Z1 = refRes/ratio
            magZ1 = abs(Z1)
            phZ1 = cmath.phase(Z1)
            
            ICG[j] = magZ1
            ECG[j] = accumC
            PPG[j] = PPG_sample

            #DC = sum(ICG)/len(ICG)
            if (AC_flag):
                pass
                #ICG[j] = mags_aver[j] - DC
            j = j+1
            if (j==fulLen):
                j = 0
                ECG_filt = ECG.copy()
                ICG_filt = ICG.copy()
                if (filter_50_mode):
                    #ECG_aver = signal.filtfilt(b50, a50, ECG_aver) #50Hz remove
                    for m in range ((fulLen)-4):
                        ECG_filt[m]  = (ECG[m] + ECG[m+1] + ECG[m+2] + ECG[m+3])/4
                        ICG_filt[m]  = (ICG[m] + ICG[m+1] + ICG[m+2] + ICG[m+3])/4
                        #leave end case
                if (filter_100_mode):
                    for m in range ((fulLen)-2):
                        ECG_filt[m]  = (ECG[m] + ECG[m+1])/2
                        ICG_filt[m]  = (ICG[m] + ICG[m+1])/2
                        #leave end case

                
                if (logFlag):
                    for r in range (fulLen):
                        data4 = ["%.4f" % ICG[r],"%.4f" % ICG_filt[r],"%.4f" % ECG[r],
                        "%.4f" % ECG_filt[r], "%.4f" % PPG[r]]
                        writer.writerow(data4)

            
        if ((keyboard.is_pressed("d") or mode_button == 'd') and mode == 'e'):
            print ("d from if")
            ser.write(b'd')
            mode = 'd'
            print(header1)
            if (logFlag):
                writer.writerow(header1)
            ser.read(1400) #dummy read
        if ((keyboard.is_pressed("e") or mode_button == 'e') and mode == 'd'):
            print ("e from if")
            ser.write(b'e')
            mode = 'e'
            print(header2)
            if (logFlag):
                writer.writerow(header2)
            ser.read(1400) #dummy read
        if ((keyboard.is_pressed("1") or freq_button == 1) and freq != 1):
            ser.write(b'1')
            freq = 1
            mes = "\n\n*******Excitation signal frequency is changed to 200Hz*******\n\n"
            print(mes)
            if (logFlag):
                writer.writerow(mes)
            ser.read(1400) #dummy read
        if ((keyboard.is_pressed("2") or freq_button == 2) and freq != 2):
            ser.write(b'2')
            freq = 2
            mes = "\n\n*******Excitation signal frequency is changed to 2kHz*******\n\n"
            print(mes)
            if (logFlag):
                writer.writerow(mes)
            ser.read(1400) #dummy read
        if ((keyboard.is_pressed("3") or freq_button == 3) and freq != 3):
            ser.write(b'3')
            freq = 3
            mes = "\n\n*******Excitation signal frequency is changed to 20kHz*******\n\n"
            print(mes)
            if (logFlag):
                writer.writerow(mes)
            ser.read(1400) #dummy read            
            
        t = range(0,fulLen*5,5)
        if (j==0):
            axA.cla()
            axA.plot(t,ICG_filt[0:fulLen])
            axA.set_title('ICG', fontweight ="bold")            

            axB.cla()
            axB.plot(t,ECG_filt[0:fulLen])
            axB.set_title('ECG', fontweight ="bold")
            
            axC.cla()
            axC.plot(t,PPG[0:fulLen])
            axC.set_title('PPG', fontweight ="bold")

            plt.xlabel ("time (ms)")
            plt.draw()
            plt.pause(0.1)
    
    i = 0
 
f.close()      
input('Press enter to exit')
