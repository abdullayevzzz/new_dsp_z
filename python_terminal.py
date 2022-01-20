refRes = complex(9985.0, 0.0) #reference resistor OHM
import serial
import keyboard
import cmath
import math
import csv
import struct
print ("Serial interface is open\n")
f = open('logs/log.csv', 'w', newline='')
writer = csv.writer(f)

ser = serial.Serial('COM6', 230400, timeout=1)
#readByte = ser.read(1)
buffer = ser.read(12)
i = 0
while not (buffer[i] == 0x7F and buffer[(i+1, 0)[i+1 == 12]] == 0xFF):
    i = i+1    #find syncronization
ser.read(i) #throw away until next syncronisation
#now ser.read is synchronised
mags = [0] * 12
i = 0

freq = 1 #(10^freq-1)kHz default frequency
ser.write(b'1')
#ser.write(chr(freq))
mes = ['Default excitation signal frequency is ' + str(10**(freq-1)) + 'kHz']
print(mes)
ser.read(1200)

mode = 'e' #default mode
ser.write(b'e')
ser.read(1200)

header1 = ['Magnitude of Z', 'Phase of Z', 'Real of Z', 'Imaginary of Z', 'Real Excit', 'Imag Excit', 'Real Resp', 'Imag Resp']

header2 = ['Magnitude of Z', 'Phase of Z', 'Real of Z', 'Imaginary of Z', 'Ratio Real', 'Ratio Imag']

if mode == 'd':
    print(header1)
    writer.writerow(header1)
elif mode == 'e':
    print(header2)
    writer.writerow(header2)

while not keyboard.is_pressed("s"):
    buffer = ser.read(12)
    if (buffer[0] == 0x7F and buffer[1] == 0xFF):   #confirm synchronization
        if mode == 'd':
            packet_number = int.from_bytes(buffer[2:4], byteorder='little')
            accumA1I = int.from_bytes(buffer[4:6], byteorder='little', signed = "True")
            accumA1Q = int.from_bytes(buffer[6:8], byteorder='little', signed = "True")
            accumB1I = int.from_bytes(buffer[8:10], byteorder='little', signed = "True")
            accumB1Q = int.from_bytes(buffer[10:12], byteorder='little', signed = "True")
            accumA1 = complex (accumA1I,accumA1Q) 
            accumB1 = complex (accumB1I,accumB1Q)
            if (accumA1 == accumB1):
                accumA1 = accumA1+1
            Z1 = accumB1*refRes/(accumA1-accumB1)
            #magZ1 = abs(accumB1)*refRes/(abs(accumA1)-abs(accumB1))
            magZ1 = abs(Z1)
            phZ1 = cmath.phase(Z1)
            mags[i%12] = magZ1 # circular add
            i = i+1
            if (i<12):
                mags_average = sum(mags)/i
            else:
                mags_average = sum(mags)/12
            data = (str("%.4f" % magZ1) + ', ' + str("%.4f" % phZ1) + ', ' + str("%.4f" % Z1.real) + \
            ', ' + str("%.4f" % Z1.imag) + ', ' + str(accumA1I) + ', ' + str(accumA1Q) + ', ' + str(accumB1I) + \
            ', ' + str(accumB1Q)) # print row
            print (data)
            data2 = ["%.4f" % magZ1,"%.4f" % phZ1,"%.4f" % Z1.real,"%.4f" % Z1.imag,accumA1I,accumA1Q,accumB1I,accumB1Q]
            writer.writerow(data2)
            
        elif mode == 'e':
            packet_number = int.from_bytes(buffer[2:4], byteorder='little')
            
            ratioReal = struct.unpack('<f', buffer[4:8])
            ratioImag = struct.unpack('<f', buffer[8:12])
            
            ratio = complex(ratioReal[0],ratioImag[0])
            Z1 = refRes/(ratio-1)
            magZ1 = abs(Z1)
            phZ1 = cmath.phase(Z1)
            
            data3 = (str("%.4f" % magZ1) + ', ' + str("%.4f" % phZ1) + ', ' + str("%.4f" % Z1.real) + \
            ', ' + str("%.4f" % Z1.imag) + ', ' + str("%.8f" % ratioReal[0]) + ', ' + str("%.8f" % ratioImag[0]))
            print (data3)
            data4 = ["%.4f" % magZ1, "%.4f" % phZ1, "%.4f" % Z1.real, "%.4f" % Z1.imag, "%.8f" % ratioReal[0], "%.8f" % ratioImag[0]]
            writer.writerow(data4)
            
        if (keyboard.is_pressed("d") and mode == 'e'):
            ser.write(b'd')
            mode = 'd'
            print(header1)
            writer.writerow(header1)
            ser.read(1200) #dummy read
        if (keyboard.is_pressed("e") and mode == 'd'):
            ser.write(b'e')
            mode = 'e'
            print(header2)
            writer.writerow(header2)
            ser.read(1200) #dummy read
        if (keyboard.is_pressed("1") and freq != 1):
            ser.write(b'1')
            freq = 1
            mes = "\n\n*******Excitation signal frequency is changed to 1kHz*******\n\n"
            print(mes)
            writer.writerow(mes)
            ser.read(1200) #dummy read
        if (keyboard.is_pressed("2") and freq != 2):
            ser.write(b'2')
            freq = 2
            mes = "\n\n*******Excitation signal frequency is changed to 10kHz*******\n\n"
            print(mes)
            writer.writerow(mes)
            ser.read(1200) #dummy read
        if (keyboard.is_pressed("3") and freq != 3):
            ser.write(b'3')
            freq = 3
            mes = "\n\n*******Excitation signal frequency is changed to 100kHz*******\n\n"
            print(mes)
            writer.writerow(mes)
            ser.read(1200) #dummy read
            
    i = 0
    while not (buffer[i] == 0x7F and buffer[(i+1, 0)[i+1 == 12]] == 0xFF):
        i = i+1    #find syncronization
    ser.read(i) #throw away until next syncronisation
 
f.close()      
input('Press enter to exit')
