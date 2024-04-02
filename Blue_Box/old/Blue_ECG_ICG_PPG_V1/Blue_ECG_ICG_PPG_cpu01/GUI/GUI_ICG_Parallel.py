refRes = complex(909.0, 0.0)  # reference resistor OHM
mux_time_ms = 1000
packet_size = 24

from collections import deque
from scipy import signal
import serial
import serial.tools.list_ports
import keyboard
import cmath
import math
import csv
import struct
import time
from matplotlib import pyplot as plt
from matplotlib.widgets import Button, RadioButtons, CheckButtons, TextBox

# FIFOQueue class definition
class FIFOQueue:
    def __init__(self, size):
        self.size = size
        self.raw = deque([0] * size, maxlen=size)
        self.downsampled = deque([0] * (size // 5), maxlen=size // 5)
        self.filter_50 = deque([0] * 4, maxlen=4)  # window for 50Hz moving average filter
        self.filter_100 = deque([0] * 2, maxlen=2)  # window for 100Hz moving average filter
        self.temp = []  # Temporary buffer for downsampling
        self.prev = 0
        self.prev_prev = 0

    def add(self, value):
        if filter_median_mode:
            # Median filtering
            if (self.prev_prev <= value <= self.prev) or (self.prev <= value <= self.prev_prev):
                median_filtered = value
            elif (self.prev_prev <= self.prev <= value) or (value <= self.prev <= self.prev_prev):
                median_filtered = self.prev
            else:
                median_filtered = self.prev_prev
            self.prev_prev = self.prev
            self.prev = value
            value = median_filtered

        self.raw.append(value)
        self.temp.append(value)
        # Averaging and PLI filtering
        if len(self.temp) == 5:
            average = sum(self.temp) / 5
            self.temp = []  # Clear the temporary buffer
            self.downsampled.append(average)
            if filter_50_mode:
                self.filter_50.append(self.downsampled[-1])
                self.downsampled[-1] = sum(self.filter_50) / 4
                # self.downsampled[-1] = (self.downsampled[-4] + self.downsampled[-3] + self.downsampled[-2] + self.downsampled[-1])/4
            if filter_100_mode:
                self.filter_100.append(self.downsampled[-1])
                self.downsampled[-1] = sum(self.filter_100) / 2
                # self.downsampled[-1] = (self.downsampled[-2] + self.downsampled[-1])/2
            return self.downsampled[-1]


class RangeSetter:
    def __init__(self):
        self.coefs = ['x0', 'x0.25', 'x0.5', 'x1', 'x2', 'x4', 'x8', 'x16']
        self.ranges = {'EXC': 3, 'Z 1': 7, 'Z 2': 7}
        self.current_signal = 'EXC'
    def next(self, event=0):
        coef_index = self.ranges[self.current_signal]
        coef_index = coef_index + 1 if coef_index < 7 else 0
        self.ranges[self.current_signal] = coef_index
        textbox.set_val(self.coefs[coef_index])
        if self.current_signal == 'EXC':
            ser.write(b'X')
        elif self.current_signal == 'Z 1':
            ser.write(b'A')
        elif self.current_signal == 'Z 2':
            ser.write(b'B')
        ser.read(packet_size * 250)
        # fig.canvas.draw_idle()
    def prev(self, event=0):
        coef_index = self.ranges[self.current_signal]
        coef_index = coef_index - 1 if coef_index > 0 else 7
        self.ranges[self.current_signal] = coef_index
        textbox.set_val(self.coefs[coef_index])
        if self.current_signal == 'EXC':
            ser.write(b'x')
        elif self.current_signal == 'Z 1':
            ser.write(b'a')
        elif self.current_signal == 'Z 2':
            ser.write(b'b')
        ser.read(packet_size * 250)
        # fig.canvas.draw_idle()
    def cur(self):
        return self.coefs[self.ranges[self.current_signal]]
    def coef_float(self, signal_name):
        return float(self.coefs[self.ranges[signal_name]][1:])
    def change_cur_signal(self, signal_name):
        self.current_signal = signal_name

signal_ranges = RangeSetter()

# Set default filter modes
filter_50_mode = True
filter_100_mode = True
filter_median_mode = True

# Initialize FIFO queues
fulLen = 5000
ECG_data = FIFOQueue(fulLen)
BIOZ1_data = FIFOQueue(fulLen)
BIOZ2_data = FIFOQueue(fulLen)
BIOZ3_data = FIFOQueue(fulLen)
BIOZ4_data = FIFOQueue(fulLen)
ECG_data = FIFOQueue(fulLen)
PPG_RED_data = FIFOQueue(fulLen)

DC = 0  # default dc value
DC_len = 50  # Length of hysteresis to calculate dc. Must be smaller than fulLen/4
AC_flag = False

mode_button = 'd'  # default mode
freq_button = 2  # (10^freq-1)kHz default frequency

logFlag = False

# plt.figure(figsize=(8, 6), dpi=100)
# axA = plt.axes([0.1, 0.80, 0.8, 0.15])
# axA.plot(1)
# axB = plt.axes([0.1, 0.55, 0.8, 0.15])
# axB.plot(1)

# axC = plt.axes([0.1, 0.30, 0.8, 0.15])
# axC.plot(1)

# Design notch filters
fs = 200.0  # Sample frequency (Hz)
f1 = 50.0  # Frequency to be removed from signal (Hz)
f2 = 99.99  # Frequency to be removed from signal (Hz)
Q = 30.0  # Quality factor
b50, a50 = signal.iirnotch(f1, Q, fs)
b100, a100 = signal.iirnotch(f2, Q, fs)


def filters(labels):
    global filter_50_mode
    global filter_100_mode
    global filter_median_mode
    if (labels == '50Hz'):
        filter_50_mode = not filter_50_mode
    if (labels == '100Hz'):
        filter_100_mode = not filter_100_mode
    if (labels == 'Median'):
        filter_median_mode = not filter_median_mode


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
    exit()


def log(event):
    global logFlag
    global writer
    logFlag = not logFlag
    if logFlag:
        timestr = time.strftime("%Y%m%d_%H%M%S")
        f = open('log_' + timestr + '.csv', 'w+', newline='')
        writer = csv.writer(f)
        header0 = ['BIOZ1', 'BIOZ2', 'BIOZ3', 'BIOZ4', 'ECG', 'PPG', 'EXC_FREQ_KHZ', 'MUX_CONFIG', 'PACKET_NUMBER']
        writer.writerow(header0)
    else:
        try:
            f
        except NameError:
            pass
        else:
            f.close()
            writer.close()


def DC_AC(event):
    global AC_flag
    AC_flag = not AC_flag

fig, (axA, axB, axC, axD, axE, axF) = plt.subplots(6, 1, figsize=(10, 8), dpi=100, sharex=True)
plt.subplots_adjust(top=0.95, left=0.15, bottom=0.30, hspace=0.3)
t = list(range(0, fulLen, 5))
line1, = axA.plot(t, list(BIOZ1_data.downsampled))
axA.set_title('BIOZ 1', fontweight="bold")
line2, = axB.plot(t, list(BIOZ2_data.downsampled))
axB.set_title('BIOZ 2', fontweight="bold")
line3, = axC.plot(t, list(BIOZ3_data.downsampled))
axC.set_title('BIOZ 3', fontweight="bold")
line4, = axD.plot(t, list(BIOZ4_data.downsampled))
axD.set_title('BIOZ 4', fontweight="bold")
line5, = axE.plot(t, list(ECG_data.downsampled))
axE.set_title('ECG', fontweight="bold")
line6, = axF.plot(t, list(PPG_RED_data.downsampled))
axF.set_title('PPG', fontweight="bold")

ax1 = plt.axes([0.1, 0.15, 0.1, 0.1])
b1 = Button(ax1, '1kHz', color="yellow")
b1.on_clicked(freq_1)

ax2 = plt.axes([0.25, 0.15, 0.1, 0.1])
b2 = Button(ax2, '10kHz', color="yellow")
b2.on_clicked(freq_2)

ax3 = plt.axes([0.40, 0.15, 0.1, 0.1])
b3 = Button(ax3, '100kHz', color="yellow")
b3.on_clicked(freq_3)

ax7 = plt.axes([0.55, 0.15, 0.1, 0.1])
labels = ['50Hz', '100Hz', 'Median']
b7 = CheckButtons(ax7, labels, (True, True, True))
b7.on_clicked(filters)

ax4 = plt.axes([0.1, 0.02, 0.1, 0.1])
b4 = Button(ax4, 'Decimal', color="yellow")
b4.on_clicked(mode_d)

ax5 = plt.axes([0.25, 0.02, 0.1, 0.1])
b5 = Button(ax5, 'Floating', color="yellow")
b5.on_clicked(mode_e)

ax6 = plt.axes([0.40, 0.02, 0.1, 0.1])
b6 = Button(ax6, 'DC/AC', color="yellow")
b6.on_clicked(DC_AC)

ax8 = plt.axes([0.70, 0.02, 0.1, 0.1])
b8 = Button(ax8, 'Stop', color="yellow")
b8.on_clicked(stop)

ax9 = plt.axes([0.55, 0.02, 0.1, 0.1])
b9 = Button(ax9, 'Log', color="yellow")
b9.on_clicked(log)

# Range buttons
ax10 = plt.axes([0.70, 0.22, 0.05, 0.03])
b10 = Button(ax10, '+', color="yellow")
b10.on_clicked(signal_ranges.next)

ax11 = plt.axes([0.70, 0.18, 0.05, 0.04], frameon=True)
textbox = TextBox(ax11, '', initial=signal_ranges.cur())
textbox.set_val(signal_ranges.cur())  # Set the initial text value
textbox.cursor.set_color('white')

ax12 = plt.axes([0.70, 0.15, 0.05, 0.03])
b12 = Button(ax12, '-', color="yellow")
b12.on_clicked(signal_ranges.prev)

ax13 = plt.axes([0.75, 0.15, 0.05, 0.1], facecolor='white')
radio = RadioButtons(ax13, ('EXC', 'Z 1', 'Z 2'))
for circle in radio.circles:
    circle.set_radius(0.08)  # Increase the radius (size) of each radio button

def choose_range_signal(label):
    signal_ranges.change_cur_signal(label)
    textbox.set_val(signal_ranges.cur())
    print(label)
    # fig.canvas.draw_idle()
radio.on_clicked(choose_range_signal)

## Initial plot
plt.ion()
plt.show(block=False)
plt.pause(0.1)

# Automatic port finder
port_name = ''
ports = list(serial.tools.list_ports.comports())
for p in ports:
    if 'XDS100' in p[1]:
        print("Device found")
        port_name = p[0]
        global ser
        ser = serial.Serial(port_name, 460800, timeout=1)
        break


### DEBUGGG
class ser_debugg:
    def __init__(self):
        self.buffer = [0x7F, 0xFF, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
        self.i = 0
        self.previous_time = time.time()
    def read(self, n):
        current_time = time.time()
        while current_time - self.previous_time < 0.0001:
            current_time = time.time()
        temp = []
        for i in range(n):
            temp.append(self.buffer[i % packet_size])
            i += 1
        self.previous_time = current_time
        return temp
    def write(self, n):
        return 1
    def flush(self):
        pass
# ser = ser_debugg()
### DEBUGGG

if not ser:
    print("Device not found")
    exit()

ser.read(packet_size * 100)
ser.write(b'i') # initialize

freq = 2  # (10^freq-1)kHz default frequency
ser.write(b'2')
# ser.write(chr(freq))
mes = ['Default excitation signal frequency is ' + str(10 ** (freq - 1)) + 'kHz']
print(mes)
ser.read(packet_size * 100)

mode = 'd'  # default mode
ser.write(b'd')
ser.read(packet_size * 100)

header1 = ['BIOZ1', 'BIOZ2', 'BIOZ3', 'BIOZ4']

header2 = ['BIOZ1', 'BIOZ2', 'BIOZ3', 'BIOZ4']

if mode == 'd':
    print(header1)
    # writer.writerow(header1)
elif mode == 'e':
    print(header2)
    # writer.writerow(header2)

ser.flush()
# readByte = ser.read(1)
buffer = ser.read(packet_size)
while len(buffer) != packet_size:
    buffer = ser.read(packet_size)
i = 0
while not (buffer[i] == 0x7F and buffer[(i + 1) % packet_size] == 0xFF):
    i = i + 1  # find syncronization
    if i == packet_size:
        print('Synchronization not found')
        print(buffer)
        buffer = ser.read(packet_size)
        i = 0
        continue
ser.read(i)  # throw away until next syncronisation
# now ser.read is synchronised
i = 0

circular_counter = 0
mux_counter = 0
mux_mode_cur = 0
mux_mode_prev = 0
while not keyboard.is_pressed("s"):
    buffer = ser.read(packet_size)
    if len(buffer) != packet_size:
        print("Buffer not read")
        continue
    elif (buffer[0] == 0x7F and buffer[1] == 0xFF):  # confirm synchronization
        if mode == 'd':
            # packet_number = int.from_bytes(buffer[2:4], byteorder='little')
            accumA1I = int.from_bytes(buffer[2:4], byteorder='little', signed="True")
            accumA1Q = int.from_bytes(buffer[4:6], byteorder='little', signed="True")
            accumB1I = int.from_bytes(buffer[6:8], byteorder='little', signed="True")
            accumB1Q = int.from_bytes(buffer[8:10], byteorder='little', signed="True")
            accumC1I = int.from_bytes(buffer[10:12], byteorder='little', signed="True")
            accumC1Q = int.from_bytes(buffer[12:14], byteorder='little', signed="True")
            accumA1 = complex(accumA1I, accumA1Q)
            accumB1 = complex(accumB1I, accumB1Q)
            accumC1 = complex(accumC1I, accumC1Q)
            if accumA1 == 0:  # Skip zero division
                print("Sample skipped")
                continue
            try:
                Z1 = (signal_ranges.coef_float('EXC') / signal_ranges.coef_float('Z 1')) * (accumB1 * refRes / accumA1)
            except ZeroDivisionError:
                Z1 = 0
            try:
                Z2 = (signal_ranges.coef_float('EXC')  / signal_ranges.coef_float('Z 2')) * (accumC1 * refRes / accumA1)
            except ZeroDivisionError:
                Z2 = 0

        elif mode == 'e':
            # packet_number = int.from_bytes(buffer[2:4], byteorder='little')
            ratioReal = struct.unpack('<f', buffer[2:6])
            ratioImag = struct.unpack('<f', buffer[6:10])
            ratioReal2 = struct.unpack('<f', buffer[10:14])
            ratioImag2 = struct.unpack('<f', buffer[14:18])
            ratio = complex(ratioReal[0], ratioImag[0])
            ratio2 = complex(ratioReal2[0], ratioImag2[0])
            if (ratio == 0) or (ratio2 == 0):  # Skip zero division
                print("Sample skipped")
                continue
            try:
                Z1 = (signal_ranges.coef_float('EXC') / signal_ranges.coef_float('Z 1')) * (refRes / ratio)
            except ZeroDivisionError:
                Z1 = 0
            try:
                Z2 = (signal_ranges.coef_float('EXC')  / signal_ranges.coef_float('Z 2')) * (refRes / ratio2)
            except ZeroDivisionError:
                Z2 = 0
        
        accumD = int.from_bytes(buffer[18:20], byteorder='little', signed="False")
        ppg_red = int.from_bytes(buffer[20:22], byteorder='little', signed="False") & 0xFFFF
        mux_mode_cur = int.from_bytes(buffer[22:23], byteorder='little', signed="False") & 0x0F
        packet_number = int.from_bytes(buffer[23:24], byteorder='little', signed="False") & 0xFF
        
        magZ1 = abs(Z1)
        phZ1 = cmath.phase(Z1)
        magZ2 = abs(Z2)
        phZ2 = cmath.phase(Z2)
        
        # Add values to raw queues and process for downsampling
        ECG_data.add(accumD)
        PPG_RED_data.add(ppg_red)
        if mux_mode_cur == 1:
            if mux_mode_cur != mux_mode_prev:
                mux_mode_prev = mux_mode_cur
                # ser.read(packet_size * 4) # throw away some samples
                continue # skip this sample
            BIOZ1_data.add(magZ1)
            BIOZ2_data.add(magZ2)
        elif mux_mode_cur == 2:
            if mux_mode_cur != mux_mode_prev:
                mux_mode_prev = mux_mode_cur
                # ser.read(packet_size * 4) # throw away some samples
                continue # skip this sample
            BIOZ3_data.add(magZ1)
            BIOZ4_data.add(magZ2)
        else:
            # ser.read(packet_size * 4) # throw away some samples
            continue
        

    else:
        print('Synchronization not confirmed. Use previous values')
        i = 0
        while not (buffer[i] == 0x7F and buffer[(i + 1) % packet_size] == 0xFF):  # modify code, make unique sync. code
            i = i + 1  # find syncronization
            if i == packet_size:
                print('Synchronization not found')
                break
        ser.read(i)  # throw away until next syncronisation

    if ((keyboard.is_pressed("d") or mode_button == 'd') and mode == 'e'):
        print("d from if")
        ser.write(b'd')
        mode = 'd'
        print(header1)
        if (logFlag):
            writer.writerow(header1)
        ser.read(packet_size * 100)  # dummy read
    elif ((keyboard.is_pressed("e") or mode_button == 'e') and mode == 'd'):
        print("e from if")
        ser.write(b'e')
        mode = 'e'
        print(header2)
        if (logFlag):
            writer.writerow(header2)
        ser.read(packet_size * 100)  # dummy read
    elif ((keyboard.is_pressed("1") or freq_button == 1) and freq != 1):
        ser.write(b'1')
        freq = 1
        mes = "\n\n*******Excitation signal frequency is changed to 1kHz*******\n\n"
        print(mes)
        if (logFlag):
            #   writer.writerow(mes)
            pass
        ser.read(packet_size * 10)  # dummy read
    elif ((keyboard.is_pressed("2") or freq_button == 2) and freq != 2):
        ser.write(b'2')
        freq = 2
        mes = "\n\n*******Excitation signal frequency is changed to 10kHz*******\n\n"
        print(mes)
        if (logFlag):
            #   writer.writerow(mes)
            pass
        ser.read(packet_size * 10)  # dummy read
    elif ((keyboard.is_pressed("3") or freq_button == 3) and freq != 3):
        ser.write(b'3')
        freq = 3
        mes = "\n\n*******Excitation signal frequency is changed to 100kHz*******\n\n"
        print(mes)
        if (logFlag):
            #   writer.writerow(mes)
            pass
        ser.read(packet_size * 10)  # dummy read

    if logFlag:
        writer.writerow([BIOZ1_data.raw[-1], BIOZ2_data.raw[-1], BIOZ3_data.raw[-1], BIOZ4_data.raw[-1],
        ECG_data.raw[-1], PPG_RED_data.raw[-1], str(10**(freq-1)), mux_mode_cur, packet_number])

    circular_counter += 1
    if (circular_counter >= 300):
        circular_counter = 0
        line1.set_ydata(list(BIOZ1_data.downsampled))
        axA.set_ylim(min(BIOZ1_data.downsampled), max(BIOZ1_data.downsampled))
        line2.set_ydata(list(BIOZ2_data.downsampled))
        axB.set_ylim(min(BIOZ2_data.downsampled), max(BIOZ2_data.downsampled))
        line3.set_ydata(list(BIOZ3_data.downsampled))
        axC.set_ylim(min(BIOZ3_data.downsampled), max(BIOZ3_data.downsampled))
        line4.set_ydata(list(BIOZ4_data.downsampled))
        axD.set_ylim(min(BIOZ4_data.downsampled), max(BIOZ4_data.downsampled))
        line5.set_ydata(list(ECG_data.downsampled))
        axE.set_ylim(min(ECG_data.downsampled), max(ECG_data.downsampled))
        line6.set_ydata(list(PPG_RED_data.downsampled))
        axF.set_ylim(min(PPG_RED_data.downsampled), max(PPG_RED_data.downsampled))
        fig.canvas.draw_idle()
        plt.pause(0.1)
        
input('Press enter to exit')
