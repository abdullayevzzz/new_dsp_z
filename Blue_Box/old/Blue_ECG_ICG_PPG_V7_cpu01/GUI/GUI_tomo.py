import random

refRes = complex(10000.0, 0.0)  # reference resistor OHM
mux_time_ms = 100
packet_size = 28
num_z_channels = 2  # default number of BIOZ channels
mode_button = 'd'  # default mode
freq_button = 2  # (10^freq-1)kHz default frequency
tomo_mode = False
tomo_mux_updated = False
tomo_exc_updated = False

from collections import deque
from scipy import signal
import serial
import serial.tools.list_ports
import cmath
import math
import csv
import struct
import time
import warnings
import threading
import numpy as np
from matplotlib import pyplot as plt
from matplotlib.widgets import Button, RadioButtons, CheckButtons, TextBox


class MuxLists:
    def __init__(self):
        self.raw = []
        self.filtered = []
        self.temp = deque(maxlen = 3)
        self.filtered_sum = 0

    def add(self, value):
        self.raw.append(value)
        self.temp.append(value)
        if len(self.temp) >= 3:
            self.filtered.append(sorted(self.temp)[1])
            self.filtered_sum += self.filtered[-1]

    def clear(self):
        self.raw.clear()
        self.filtered.clear()
        self.temp = deque(maxlen = 3)
        self.filtered_sum = 0

    def average(self):
        if len(self.filtered):
            return self.filtered_sum / len(self.filtered)
        else:
            return 0

# FIFOQueue class definition
class FIFOQueue:
    def __init__(self, size):
        self.size = size
        self.raw = deque([0] * self.size, maxlen=self.size)
        self.downsampled = deque([0] * (self.size // 5), maxlen=self.size // 5)
        self.filter_50 = deque([0] * 4, maxlen=4)
        self.filter_100 = deque([0] * 2, maxlen=2)
        self.temp = []
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
            if filter_100_mode:
                self.filter_100.append(self.downsampled[-1])
                self.downsampled[-1] = sum(self.filter_100) / 2
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
BIOZ1_data = FIFOQueue(fulLen)
BIOZ2_data = FIFOQueue(fulLen)
BIOZ3_data = FIFOQueue(fulLen)
BIOZ4_data = FIFOQueue(fulLen)
BIOZ5_data = FIFOQueue(fulLen)
BIOZ6_data = FIFOQueue(fulLen)
BIOZ7_data = FIFOQueue(fulLen)
BIOZ8_data = FIFOQueue(fulLen)
ECG_data = FIFOQueue(fulLen)
PPG_RED_data = FIFOQueue(fulLen)
Z1_TOMO_MAG = MuxLists()
Z2_TOMO_MAG = MuxLists()
Z1_TOMO_PHASE = MuxLists()
Z2_TOMO_PHASE = MuxLists()
Z1_TOMO_MAG_AVG = 0
Z2_TOMO_MAG_AVG = 0
Z1_TOMO_PHASE_AVG = 0
Z2_TOMO_PHASE_AVG = 0

DC = 0  # default dc value
DC_len = 50  # Length of hysteresis to calculate dc. Must be smaller than fulLen/4
AC_flag = False
logFlag = False


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
        header_wave = ['BIOZ1', 'BIOZ2', 'BIOZ3', 'BIOZ4', 'ECG', 'PPG', 'EXC_FREQ_KHZ', 'MUX_CONFIG', 'PACKET_NUMBER']
        header_tomo = ['BIOZ1_MAG', 'BIOZ1_PHASE', 'EXC_FREQ_KHZ', 'EXC_PIN', 'GND_PIN', 'SNS_AP_PIN', 'SNS_AN_PIN', 'PACKET_NUMBER']
        if tomo_mode is False:
            writer.writerow(header_wave)
        if tomo_mode is True:
            writer.writerow(header_tomo)
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


def choose_range_signal(label):
    signal_ranges.change_cur_signal(label)
    textbox.set_val(signal_ranges.cur())
    print(label)


def choose_wave_tomo(label):
    global tomo_mode
    if label == 'Tomo':
        ser.write(b'o')
        tomo_mode = True
    else:
        ser.write(b'n')
        tomo_mode = False
    print(tomo_mode)


def choose_channel_count(label):
    global num_z_channels
    num_z_channels = int(label[0])
    print(label)


def create_wave_figure():
    # global fig, axA, axB, axC, axD, axE, axF, t, line1, line2, line3, line4, line5, line6
    global fig, axs, t, lines
    if num_z_channels == 2:
        subplots_config = (4, 1)
    elif num_z_channels == 4:
        subplots_config = (3, 2)
    elif num_z_channels == 8:
        subplots_config = (5, 2)
    fig, axs = plt.subplots(*subplots_config, figsize=(10, 8), dpi=100, sharex=True)
    plt.subplots_adjust(top=0.95, left=0.15, bottom=0.30, hspace=0.3)
    axs = axs.flatten()
    axs = axs[:num_z_channels + 2]  # Add two more channels for ECG and PPG
    t = list(range(0, fulLen, 5))
    lines = []
    for ax in axs:
        line, = ax.plot([], [], 'r')  # Example plot
        lines.append(line)
    for i in range(num_z_channels):
        data_source = globals().get(f'BIOZ{i + 1}_data')
        lines[i], = axs[i].plot(t, list(data_source.downsampled))
        axs[i].set_title(f'BIOZ{i + 1}', fontweight="bold")

    lines[-2], = axs[-2].plot(t, list(ECG_data.downsampled))
    axs[-2].set_title('ECG', fontweight="bold")
    lines[-1], = axs[-1].plot(t, list(PPG_RED_data.downsampled))
    axs[-1].set_title('PPG', fontweight="bold")

    plt.ion()
    plt.show(block=False)
    plt.pause(0.1)


def create_tomo_figure():
    # global fig, axA, axB, axC, axD, axE, axF, t, line1, line2, line3, line4, line5, line6
    global fig, axs, t, all_bars
    fig, axs = plt.subplots(4, 4, figsize=(10, 8), dpi=100, sharex=True)
    plt.subplots_adjust(top=0.95, left=0.15, bottom=0.30, hspace=0.3)
    axs = axs.flatten()

    # Generate initial data
    initial_data = np.zeros((16, 16))
    # Plot the initial data
    all_bars = []
    for ax, data in zip(axs, initial_data):
        fixed_excitation_bars = ax.bar(range(0, 16), data)
        all_bars.append(fixed_excitation_bars)
    # axs = axs[:1 + 2]  # Add two more channels for ECG and PPG
    # t = list(range(0, fulLen, 5))
    #lines = []
    #for ax in axs:
    #    line, = ax.plot([], [], 'r')  # Example plot
    #    lines.append(line)
    # for i in range(num_z_channels):
    #     data_source = globals().get(f'BIOZ{i + 1}_data')
    #     lines[i], = axs[i].plot(t, list(data_source.downsampled))
    #     axs[i].set_title(f'BIOZ{i + 1}', fontweight="bold")

    # lines[-2], = axs[-2].plot(t, list(ECG_data.downsampled))
    # axs[-2].set_title('ECG', fontweight="bold")
    # lines[-1], = axs[-1].plot(t, list(PPG_RED_data.downsampled))
    # axs[-1].set_title('PPG', fontweight="bold")

    plt.ion()
    plt.show(block=False)
    plt.pause(0.1)


def create_common_buttons():
    global button_axs, buttons, textbox
    button_axs = []
    buttons = []

    button_axs.append(plt.axes([0.1, 0.15, 0.1, 0.1]))
    buttons.append(Button(button_axs[-1], '1kHz', color="yellow"))
    buttons[-1].on_clicked(freq_1)

    button_axs.append(plt.axes([0.25, 0.15, 0.1, 0.1]))
    buttons.append(Button(button_axs[-1], '10kHz', color="yellow"))
    buttons[-1].on_clicked(freq_2)

    button_axs.append(plt.axes([0.40, 0.15, 0.1, 0.1]))
    buttons.append(Button(button_axs[-1], '100kHz', color="yellow"))
    buttons[-1].on_clicked(freq_3)

    button_axs.append(plt.axes([0.1, 0.02, 0.1, 0.1]))
    buttons.append(Button(button_axs[-1], 'Decimal', color="yellow"))
    buttons[-1].on_clicked(mode_d)

    button_axs.append(plt.axes([0.25, 0.02, 0.1, 0.1]))
    buttons.append(Button(button_axs[-1], 'Floating', color="yellow"))
    buttons[-1].on_clicked(mode_e)

    button_axs.append(plt.axes([0.85, 0.02, 0.1, 0.1]))
    buttons.append(Button(button_axs[-1], 'Stop', color="yellow"))
    buttons[-1].on_clicked(stop)

    button_axs.append(plt.axes([0.55, 0.02, 0.1, 0.1]))
    buttons.append(Button(button_axs[-1], 'Log', color="yellow"))
    buttons[-1].on_clicked(log)

     # Range buttons
    button_axs.append(plt.axes([0.70, 0.09, 0.05, 0.03]))
    buttons.append(Button(button_axs[-1], '+', color="yellow"))
    buttons[-1].on_clicked(signal_ranges.next)

    button_axs.append(plt.axes([0.70, 0.05, 0.05, 0.04], frameon=True))
    textbox = TextBox(button_axs[-1], '', initial=signal_ranges.cur())
    textbox.set_val(signal_ranges.cur())  # Set the initial text value
    textbox.cursor.set_color('white')
    buttons.append(textbox)

    button_axs.append(plt.axes([0.70, 0.02, 0.05, 0.03]))
    buttons.append(Button(button_axs[-1], '-', color="yellow"))
    buttons[-1].on_clicked(signal_ranges.prev)

    button_axs.append(plt.axes([0.75, 0.02, 0.05, 0.1], facecolor='white'))
    radio = RadioButtons(button_axs[-1], ('EXC', 'Z 1', 'Z 2'))
    for circle in radio.circles:
        circle.set_radius(0.08)  # Increase the radius (size) of each radio button
    radio.on_clicked(choose_range_signal)
    buttons.append(radio)


def create_wave_buttons():
    create_common_buttons()
    global button_axs, buttons, textbox

    button_axs.append(plt.axes([0.55, 0.15, 0.1, 0.1]))
    labels = ['50Hz', '100Hz', 'Median']
    buttons.append(CheckButtons(button_axs[-1], labels, (True, True, True)))
    buttons[-1].on_clicked(filters)

    button_axs.append(plt.axes([0.40, 0.02, 0.1, 0.1]))
    buttons.append(Button(button_axs[-1], 'DC/AC', color="yellow"))
    buttons[-1].on_clicked(DC_AC)

    button_axs.append(plt.axes([0.85, 0.15, 0.1, 0.1], facecolor='white'))
    radio2 = RadioButtons(button_axs[-1], ('2_Ch BIOZ', '4_Ch BIOZ', '8_Ch BIOZ'))
    for circle in radio2.circles:  # Adjust the size of each radio button
        circle.set_radius(0.08)
    # Set initial selection based on num_z_channels
    channel_index = {2: 0, 4: 1, 8: 2}.get(num_z_channels)
    radio2.set_active(channel_index)
    radio2.on_clicked(choose_channel_count)
    buttons.append(radio2)

    button_axs.append(plt.axes([0.70, 0.15, 0.1, 0.1], facecolor='white'))
    radio3 = RadioButtons(button_axs[-1], ('Wave', 'Tomo'))
    for circle in radio3.circles:
        circle.set_radius(0.08)  # Increase the radius (size) of each radio button
    radio3.on_clicked(choose_wave_tomo)
    buttons.append(radio3)


def create_tomo_buttons():
    create_common_buttons()
    global button_axs, buttons, textbox


create_wave_figure()
create_wave_buttons()


def try_connect_bluetooth(port_device, result):
    try:
        with serial.Serial(port_device, 460800, timeout=0.5) as temp_ser:
            temp_ser.write(b'\x01')  # Send byte to trigger response
            time.sleep(0.2)  # Give some time for the device to respond
            temp_ser.reset_input_buffer()  # Clear buffer to get fresh data
            response = temp_ser.read(1)  # Attempt to read response
            if response == b'\x02':
                result.append(port_device)  # If successful, append the port to the result list
    except (serial.SerialException, OSError):
        pass  # Handle exceptions or ignore


def auto_find_port():
    ports = list(serial.tools.list_ports.comports())
    print("Searching for USB connection")
    for port in ports:
        if 'XDS100' in port.description:
            print(f"USB connection found and connected on {port.device}")
            return port.device

    print("USB connection not found, searching for Bluetooth connection")
    for port in ports:
        result = []  # To store the successful port
        connect_thread = threading.Thread(target=try_connect_bluetooth, args=(port.device, result))
        connect_thread.start()
        connect_thread.join(timeout=3)  # Set a timeout for the thread, adjust as necessary
        if result:
            print(f"Bluetooth connection found and connected on {result[0]}")
            return result[0]
    return None


### DEBUGGG
class ser_debugg:
    def __init__(self):
        self.buffer = [0x7F, 0xFF] + [1] * (packet_size - 2)
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


ser_emul = None
### DEBUGGG
# ser_emul = ser_debugg()
com_port_name = auto_find_port()
# com_port_name = 'COM7'

if ser_emul:
    print('Emulated Serial Debug')
    ser = ser_emul
elif com_port_name:
    ser = serial.Serial(com_port_name, 460800, timeout=1)
else:
    print("Device not found.")
    exit()

ser.read(packet_size * 100)
ser.write(b'i')  # initialize

freq = str(freq_button)  # (10^freq-1)kHz default frequency
ser.write(freq.encode())
mes = ['Default excitation signal frequency is ' + str(10 ** (freq_button - 1)) + 'kHz']
print(mes)
ser.read(packet_size * 100)

mode = mode_button  # default mode
ser.write(mode.encode())
ser.read(packet_size * 100)

num_z_channels_prev = num_z_channels

ser.flush()
# readByte = ser.read(1)
buffer = ser.read(packet_size)
while len(buffer) != packet_size:
    buffer = ser.read(packet_size)
    print(buffer)
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
mux_mode_cur = 0
mux_mode_prev = 0
tomo_first_time = True

while True:
    buffer = ser.read(packet_size)
    if len(buffer) != packet_size:
        print("Buffer not read")
        continue
    elif buffer[0] == 0x7F and buffer[1] == 0xFF:  # confirm synchronization
        packet_length = (int.from_bytes(buffer[2:3], byteorder='little', signed="False") >> 2) & 0xFC
        packet_type = int.from_bytes(buffer[2:3], byteorder='little', signed="False") & 0x03
        packet_number = int.from_bytes(buffer[3:4], byteorder='little', signed="False") & 0xFF
        mux_mode_temp = int.from_bytes(buffer[4:8], byteorder='little', signed="False") & 0xFFFFFFFF
        if mux_mode_temp != mux_mode_cur:
            mux_mode_prev = mux_mode_cur
            mux_mode_cur = mux_mode_temp
            tomo_mux_updated = True
            continue  # skip this sample

        if mode == 'd':
            # packet_number = int.from_bytes(buffer[2:4], byteorder='little')
            accumA1I = int.from_bytes(buffer[8:10], byteorder='little', signed="True")
            accumA1Q = int.from_bytes(buffer[10:12], byteorder='little', signed="True")
            accumB1I = int.from_bytes(buffer[12:14], byteorder='little', signed="True")
            accumB1Q = int.from_bytes(buffer[14:16], byteorder='little', signed="True")
            accumC1I = int.from_bytes(buffer[16:18], byteorder='little', signed="True")
            accumC1Q = int.from_bytes(buffer[18:20], byteorder='little', signed="True")
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
                Z2 = (signal_ranges.coef_float('EXC') / signal_ranges.coef_float('Z 2')) * (accumC1 * refRes / accumA1)
            except ZeroDivisionError:
                Z2 = 0

        elif mode == 'e':
            # packet_number = int.from_bytes(buffer[2:4], byteorder='little')
            ratioReal = struct.unpack('<f', buffer[8:12])
            ratioImag = struct.unpack('<f', buffer[12:16])
            ratioReal2 = struct.unpack('<f', buffer[16:20])
            ratioImag2 = struct.unpack('<f', buffer[20:24])
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
                Z2 = (signal_ranges.coef_float('EXC') / signal_ranges.coef_float('Z 2')) * (refRes / ratio2)
            except ZeroDivisionError:
                Z2 = 0

        accumD = int.from_bytes(buffer[24:26], byteorder='little', signed="False")
        ppg_red = int.from_bytes(buffer[26:28], byteorder='little', signed="False") & 0xFFFF

        magZ1 = abs(Z1)
        phZ1 = cmath.phase(Z1)
        magZ2 = abs(Z2)
        phZ2 = cmath.phase(Z2)

        # Add values to raw queues and process for downsampling
        ECG_data.add(accumD)
        PPG_RED_data.add(ppg_red)
        if tomo_mode is False:
            if mux_mode_cur == 1:
                if mux_mode_cur != mux_mode_prev:
                    mux_mode_prev = mux_mode_cur
                    # ser.read(packet_size * 4) # throw away some samples
                    # continue  # skip this sample
                BIOZ1_data.add(magZ1)
                BIOZ2_data.add(magZ2)
            elif mux_mode_cur == 2:
                if mux_mode_cur != mux_mode_prev:
                    mux_mode_prev = mux_mode_cur
                    # ser.read(packet_size * 4) # throw away some samples
                    # continue  # skip this sample
                BIOZ3_data.add(magZ1)
                BIOZ4_data.add(magZ2)
            elif mux_mode_cur == 3:
                if mux_mode_cur != mux_mode_prev:
                    mux_mode_prev = mux_mode_cur
                    # ser.read(packet_size * 4) # throw away some samples
                    # continue  # skip this sample
                BIOZ5_data.add(magZ1)
                BIOZ6_data.add(magZ2)
            elif mux_mode_cur == 4:
                if mux_mode_cur != mux_mode_prev:
                    mux_mode_prev = mux_mode_cur
                    # ser.read(packet_size * 4) # throw away some samples
                    # continue  # skip this sample
                BIOZ7_data.add(magZ1)
                BIOZ8_data.add(magZ2)
            else:
                continue

        if tomo_mode is True:
            # exc_pin_index = ((mux_mode_cur >> 25) & 0x1F)
            # sns_ap_index = ((mux_mode_cur >> 20) & 0x1F)
            # print('from main', exc_pin_index, sns_ap_index)
            if tomo_mux_updated is True:
                Z1_TOMO_MAG_AVG = Z1_TOMO_MAG.average()
                Z1_TOMO_PHASE_AVG = Z1_TOMO_PHASE.average()
                Z2_TOMO_MAG_AVG = Z2_TOMO_MAG.average()
                Z2_TOMO_PHASE_AVG = Z2_TOMO_PHASE.average()
                Z1_TOMO_MAG.clear()
                Z1_TOMO_MAG.clear()
                Z1_TOMO_PHASE.clear()
                Z2_TOMO_PHASE.clear()

            Z1_TOMO_MAG.add(magZ1)
            Z1_TOMO_PHASE.add(phZ1)
            Z2_TOMO_MAG.add(magZ2)
            Z2_TOMO_PHASE.add(phZ2)

    else:
        print('Synchronization not confirmed. Use previous values')
        i = 0
        while not (buffer[i] == 0x7F and buffer[(i + 1) % packet_size] == 0xFF):  # modify code, make unique sync. code
            i = i + 1  # find syncronization
            if i == packet_size:
                print('Synchronization not found')
                break
        ser.read(i)  # throw away until next syncronisation

    if mode_button == 'd' and mode == 'e':
        print("d from if")
        ser.write(b'd')
        mode = 'd'
        ser.read(packet_size * 100)  # dummy read
    elif mode_button == 'e' and mode == 'd':
        print("e from if")
        ser.write(b'e')
        mode = 'e'
        ser.read(packet_size * 100)  # dummy read
    elif freq_button == 1 and freq != 1:
        ser.write(b'1')
        freq = 1
        mes = "\n\n*******Excitation signal frequency is changed to 1kHz*******\n\n"
        print(mes)
        if logFlag:
            pass
        ser.read(packet_size * 10)  # dummy read
    elif freq_button == 2 and freq != 2:
        ser.write(b'2')
        freq = 2
        mes = "\n\n*******Excitation signal frequency is changed to 10kHz*******\n\n"
        print(mes)
        if logFlag:
            #   writer.writerow(mes)
            pass
        ser.read(packet_size * 10)  # dummy read
    elif freq_button == 3 and freq != 3:
        ser.write(b'3')
        freq = 3
        mes = "\n\n*******Excitation signal frequency is changed to 100kHz*******\n\n"
        print(mes)
        if logFlag:
            pass
        ser.read(packet_size * 10)  # dummy read
    if num_z_channels != num_z_channels_prev:
        num_z_channels_prev = num_z_channels
        ser.write({2: 'r', 4: 't', 8: 'y'}.get(num_z_channels).encode())
        plt.close('all')
        create_wave_figure()
        create_wave_buttons()

    if tomo_mode is True and tomo_first_time is True:
        plt.close('all')
        create_tomo_figure()
        create_tomo_buttons()
        tomo_first_time = False

    if logFlag:
        if tomo_mode is False:
            writer.writerow([BIOZ1_data.raw[-1], BIOZ2_data.raw[-1], BIOZ3_data.raw[-1], BIOZ4_data.raw[-1],
                             ECG_data.raw[-1], PPG_RED_data.raw[-1], str(10 ** (freq - 1)), mux_mode_cur,
                             packet_number])

        # elif (tomo_mode is True) and (tomo_mux_updated is True):  # Only log 1 averaged value per mux position
        elif all((tomo_mode, tomo_mux_updated)):
            exc_pin = ((mux_mode_prev >> 25) & 0x1F) + 1
            sns_ap_pin = ((mux_mode_prev >> 20) & 0x1F) + 1
            sns_an_pin = ((mux_mode_prev >> 15) & 0x1F) + 1
            sns_bp_pin = ((mux_mode_prev >> 10) & 0x1F) + 1
            sns_bn_pin = ((mux_mode_prev >> 5) & 0x1F) + 1
            gnd_pin = (mux_mode_prev & 0x1F) + 1

            writer.writerow([round(Z1_TOMO_MAG_AVG, 6), round(Z1_TOMO_PHASE_AVG, 6),
                             str(10 ** (freq - 1)), exc_pin, gnd_pin, sns_ap_pin, sns_an_pin, packet_number])

    circular_counter += 1
    if (circular_counter >= 300) and tomo_mode is False:
        circular_counter = 0
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", UserWarning)  # Ignore UserWarning
            for ch in range(num_z_channels):
                data_source = globals().get(f'BIOZ{ch + 1}_data')
                lines[ch].set_ydata(list(data_source.downsampled))
                axs[ch].set_ylim(min(data_source.downsampled), max(data_source.downsampled))
            lines[-2].set_ydata(list(ECG_data.downsampled))
            axs[-2].set_ylim(min(ECG_data.downsampled), max(ECG_data.downsampled))
            lines[-1].set_ydata(list(PPG_RED_data.downsampled))
            axs[-1].set_ylim(min(PPG_RED_data.downsampled), max(PPG_RED_data.downsampled))
            fig.canvas.draw_idle()
            plt.pause(0.1)

    #elif tomo_mode is True and tomo_mux_updated is True:
    elif all((tomo_mode, tomo_mux_updated)):
        tomo_mux_updated = False
        exc_pin_index = ((mux_mode_prev >> 25) & 0x1F)
        sns_ap_index = ((mux_mode_prev >> 20) & 0x1F)
        print(exc_pin_index, sns_ap_index)

        if 0 <= exc_pin_index < 16 and 0 <= sns_ap_index < 16:
            current_bar = all_bars[exc_pin_index][sns_ap_index]
            current_bar.set_height(Z1_TOMO_MAG_AVG)

        else:
            continue

        # if tomo_exc_updated and exc_pin_index == 0:
        # if tomo_exc_updated:
        # if ((mux_mode_cur >> 25) & 0x1F) == 0 and ((mux_mode_cur >> 20) & 0x1F) == 1:
        if exc_pin_index != ((mux_mode_cur >> 25) & 0x1F):
            # exc_pin_index = ((mux_mode_prev >> 25) & 0x1F)
            # sns_ap_index = ((mux_mode_prev >> 20) & 0x1F)
            print('outer', exc_pin_index, sns_ap_index)
            axs[exc_pin_index].figure.canvas.draw_idle()
            # fig.canvas.draw_idle()
            plt.pause(0.01)

input('Press enter to exit')
