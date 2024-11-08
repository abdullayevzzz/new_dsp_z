import random

refRes = complex(10000.0, 0.0)  # reference resistor OHM
packet_size = 25
num_z_channels = 2  # default number of BIOZ channels
mode_button = 'd'  # default mode
op_mode = 0
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
import os
import warnings
import threading
from matplotlib import pyplot as plt
from matplotlib.widgets import Button, RadioButtons, CheckButtons, TextBox
from matplotlib import colors
import numpy as np
import pyeit.eit.bp as bp
import pyeit.eit.protocol as protocol
import pyeit.mesh as mesh


__location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))

data_f = []
data_file_path = os.path.join(__location__, 'data.csv')
with open(data_file_path, newline='') as csvfile:
    csvreader = csv.reader(csvfile)
    next(csvreader)
    for row in csvreader:
        magnitude = float(row[0])
        phase = float(row[1])
        real_value = magnitude * math.cos(phase)
        # data_f.append(real_value)
        data_f.append(magnitude)


class EIT_reconstruct:
    def __init__(self, reference=None, use_ref=True, n_el=16, use_shape=1):
        global fig, ax1
        self.n_el = n_el  # nb of electrodes
        self.use_shape = use_shape
        self.use_ref = use_ref
        # self.fig, self.ax1 = None, None
        self.protocol_obj, self.eit = None, None
        self.reference = []
        self.ref_file_path = os.path.join(__location__, 'reference.csv')
        with open(self.ref_file_path, newline='') as csvfile:
            csvreader = csv.reader(csvfile)
            next(csvreader)
            for row in csvreader:
                magnitude = float(row[0])
                phase = float(row[1])
                real_value = magnitude * math.cos(phase)
                # self.reference.append(real_value)
                self.reference.append(magnitude)
        # self.data = self.reference

    def init_plot(self):
        global fig, ax1
        if self.use_shape == 1:
            self.mesh_obj = mesh.create(self.n_el, h0=0.05)
        self.el_pos = self.mesh_obj.el_pos
        # Create figure and axis objects for future updates
        # fig, ax1 = plt.subplots(figsize=(9, 6))
        # ax1.set_title(r"Reconstituted $\Delta$ Conductivities")
        # ax1.axis("equal")
        # ax1.set_aspect("equal")
        # ax1.set_ylim([-1.2, 1.2])
        # ax1.set_xlim([-1.2, 1.2])
        # Create EIT related variables
        self.protocol_obj = protocol.create(self.n_el, dist_exc=8, step_meas=1, parser_meas="std")
        self.eit = bp.BP(self.mesh_obj, self.protocol_obj)
        self.eit.setup(weight="simple")
        # Plot electrodes
        axs.plot(self.mesh_obj.node[self.el_pos, 0], self.mesh_obj.node[self.el_pos, 1], "ro")
        for i, e in enumerate(self.el_pos):
            axs.text(self.mesh_obj.node[e, 0], self.mesh_obj.node[e, 1], str(i + 1), size=12)
        fig.set_size_inches(8, 8)
        plt.ion()
        plt.show(block=False)

    def update_reference(self):
        pass

    def update_plot(self, data):
        # Naive inverse solver using back-projection
        referenceData = np.array(self.reference) if self.use_ref else None
        # Inverse Problem
        node_ds = self.eit.solve(data, referenceData, normalize=True)
        node_ds = np.real(node_ds)
        # Re-draw the plot with updated data
        im = axs.tripcolor(
            self.mesh_obj.node[:, 0],
            self.mesh_obj.node[:, 1],
            self.mesh_obj.element,
            node_ds,
            shading="flat",
            alpha=1,
            cmap=plt.cm.twilight_shifted,
            norm=colors.CenteredNorm()
        )
        plt.pause(0.1)  # Update the plot without blocking


class FrequencyController:
    def __init__(self, def_feq):
        self.dict = {'1 kHz': 0, '2 kHz': 1, '4 kHz': 2, '7 kHz': 3, '10 kHz': 4, '20 kHz': 5, '40 kHz': 6, '70 kHz': 7,
                     '100 kHz': 8, '200 kHz': 9, '400 kHz': 10}
        self.curr = self.dict.get(def_feq)
        self.updated = False

    def get_str_label(self):
        return list(filter(lambda x: self.dict[x] == self.curr, self.dict))[0]

    def get_int_label(self):
        return int(self.get_str_label().split(' ')[0])

    def update(self, label):
        self.curr = self.dict.get(label)
        self.updated = True


class MuxLists:
    def __init__(self):
        self.raw = []
        self.filtered = []
        self.med_fil_size = 1
        self.temp = deque(maxlen=self.med_fil_size)
        self.filtered_sum = 0
        # self.averaged_values = deque([0] * 192, maxlen=192)
        self.averaged_values = list([0] * 192)
        self.counter = 0
        # self.prev_set = deque(maxlen=192)

    def add(self, value):
        self.raw.append(value)
        self.temp.append(value)
        if len(self.temp) >= self.med_fil_size:
            self.filtered.append(sorted(self.temp)[self.med_fil_size // 2])
            self.filtered_sum += self.filtered[-1]

    def clear(self):
        self.raw.clear()
        self.filtered.clear()
        # self.temp = deque(maxlen=self.med_fil_size)
        self.temp.clear()
        self.filtered_sum = 0

    def average(self):
        if len(self.filtered):
            average = self.filtered_sum / len(self.filtered)
            # average = sum(self.raw) / len(self.raw)
            # print('filtered_sum:', self.filtered_sum, 'len:', len(self.filtered), 'average:', average)
            self.averaged_values[self.counter] = average
            self.counter += 1
            # self.prev_set.append(average)
            if self.counter >= 192:
                self.counter = 0
            return average
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
            ser.write(b'0')  # length
            # or, length can be set to 1 and another byte can set gain ('0'-'7')
            # where '0' stands for 0 gain, '1' stand for x0.25 ..., '7' for x16
            # same for 'A' and 'B'
        elif self.current_signal == 'Z 1':
            ser.write(b'A')
            ser.write(b'0')
        elif self.current_signal == 'Z 2':
            ser.write(b'B')
            ser.write(b'0')
        ser.read(packet_size * 250)
        # fig.canvas.draw_idle()

    def prev(self, event=0):
        coef_index = self.ranges[self.current_signal]
        coef_index = coef_index - 1 if coef_index > 0 else 7
        self.ranges[self.current_signal] = coef_index
        textbox.set_val(self.coefs[coef_index])
        if self.current_signal == 'EXC':
            ser.write(b'x')
            ser.write(b'0')  # length
        elif self.current_signal == 'Z 1':
            ser.write(b'a')
            ser.write(b'0')  # length
        elif self.current_signal == 'Z 2':
            ser.write(b'b')
            ser.write(b'0')  # length
        ser.read(packet_size * 250)
        # fig.canvas.draw_idle()

    def cur(self):
        return self.coefs[self.ranges[self.current_signal]]

    def coef_float(self, signal_name):
        return float(self.coefs[self.ranges[signal_name]][1:])

    def change_cur_signal(self, signal_name):
        self.current_signal = signal_name


signal_ranges = RangeSetter()
freq = FrequencyController('10 kHz')

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

# BIOZ1_C2_data = FIFOQueue(fulLen)
# BIOZ2_C2_data = FIFOQueue(fulLen)

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


def choose_freq(label):
    freq.update(label)


def stop(event):
    quit()
    exit()


def log(event):
    global logFlag
    global writer
    global __location__
    logFlag = not logFlag

    if logFlag:
        timestr = time.strftime("%Y%m%d_%H%M%S")
        log_file_path = os.path.join(__location__, 'log_' + timestr + '.csv')
        f = open(log_file_path, 'w+', newline='')
        writer = csv.writer(f)
        header_wave = ['BIOZ1', 'BIOZ2', 'BIOZ3', 'BIOZ4', 'ECG', 'PPG', 'EXC_FREQ_KHZ', 'MUX_CONFIG', 'PACKET_NUMBER']
        header_tomo = ['BIOZ1_MAG', 'BIOZ1_PHASE', 'EXC_FREQ_KHZ', 'EXC_PIN', 'GND_PIN', 'SNS_AP_PIN', 'SNS_AN_PIN',
                       'PACKET_NUMBER']
        if op_mode == 0:
            writer.writerow(header_wave)
        else:
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
    global op_mode
    if label == 'Tomo':
        ser.write(b't')
        ser.write(b'0')  # length
        op_mode = 1
    elif label == 'Tomo2':
        ser.write(b't')
        ser.write(b'0')  # length
        op_mode = 2
    elif label == 'Wave':
        ser.write(b'w')
        ser.write(b'1')  # length
        ser.write(b'2')  # 2 channel
        op_mode = 0
    print(op_mode)


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
    fig, axs = plt.subplots(*subplots_config, figsize=(8, 8), dpi=100, sharex=True)
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
    global fig, axs, t, lines
    fig, axs = plt.subplots(2, 1, figsize=(10, 8), dpi=100, sharex=True)
    plt.subplots_adjust(top=0.95, left=0.15, bottom=0.30, hspace=0.3)
    axs = axs.flatten()

    # Generate initial data
    initial_data = np.zeros((2, 1))
    t = list(range(192))
    lines = []
    for ax in axs:
        line, = ax.plot([], [], 'r')  # Example plot
        lines.append(line)
    lines[0], = axs[0].plot(t, list(Z1_TOMO_MAG.averaged_values))
    axs[0].set_title('Magnitude', fontweight="bold")
    lines[1], = axs[1].plot(t, list(Z1_TOMO_PHASE.averaged_values))
    axs[1].set_title('Phase', fontweight="bold")

    plt.ion()
    plt.show(block=False)
    plt.pause(0.1)


def create_tomo2_figure():
    # global fig, axA, axB, axC, axD, axE, axF, t, line1, line2, line3, line4, line5, line6
    global fig, axs, t, lines, reconstruct
    fig, axs = plt.subplots(figsize=(12, 10), dpi=100, sharex=True)
    plt.subplots_adjust(top=0.95, left=0.15, bottom=0.30, hspace=0.3)
    # axs = axs.flatten()
    axs.set_title(r"Reconstituted $\Delta$ Conductivities")
    axs.axis("equal")
    axs.set_aspect("equal")
    axs.set_ylim([-1.2, 1.2])
    axs.set_xlim([-1.2, 1.2])

    reconstruct = EIT_reconstruct(use_ref=True, n_el=16)
    reconstruct.init_plot()

    plt.ion()
    plt.show(block=False)
    plt.pause(0.1)


def create_common_buttons():
    global button_axs, buttons, textbox
    button_axs = []
    buttons = []

    button_axs.append(plt.axes([0.1, 0.02, 0.1, 0.23], facecolor='white'))
    radio_freq = RadioButtons(button_axs[-1],
                              ('1 kHz', '2 kHz', '4 kHz', '7 kHz', '10 kHz', '20 kHz', '40 kHz', '70 kHz',
                               '100 kHz', '200 kHz', '400 kHz'), active=4)
    for circle in radio_freq.circles:
        circle.set_radius(0.03)  # Increase the radius (size) of each radio button
    radio_freq.on_clicked(choose_freq)
    buttons.append(radio_freq)

    button_axs.append(plt.axes([0.25, 0.15, 0.1, 0.1]))
    buttons.append(Button(button_axs[-1], 'Decimal', color="yellow"))
    buttons[-1].on_clicked(mode_d)

    button_axs.append(plt.axes([0.40, 0.15, 0.1, 0.1]))
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
    radio3 = RadioButtons(button_axs[-1], ('Wave', 'Tomo', 'Tomo2'))
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
        self.buffer = [0xC7, 0xC7] + [1] * (packet_size - 2)
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

# ser.write(str(freq.curr).encode() if freq.curr < 10 else 'D'.encode())
ser.write(b'f')
ser.write(b'1')
ser.write(str(freq.curr).encode())
mes = ['Default excitation signal frequency is ' + freq.get_str_label()]
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
    # print(buffer, len(buffer))
i = 0
while not (buffer[i] == 0xC7 and buffer[(i + 1) % packet_size] == 0xC7):
    i = i + 1  # find syncronization
    if i == packet_size:
        print('Synchronization not found')
        print(buffer, len(buffer))
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
    # print('reading buffer')
    if len(buffer) != packet_size:
        print("Buffer not read")
        continue
    elif buffer[0] == 0xC7 and buffer[1] == 0xC7:  # confirm synchronization
        # print('synchronization confirmed')
        packet_length = int.from_bytes(buffer[2:3], byteorder='little', signed="False") & 0xFF
        packet_number = int.from_bytes(buffer[3:4], byteorder='little', signed="False") & 0xFF
        packet_mode = int.from_bytes(buffer[4:5], byteorder='little', signed="False") & 0x03
        mux_mode_temp = int.from_bytes(buffer[5:9], byteorder='little', signed="False") & 0xFFFFFFFF
        if mux_mode_temp != mux_mode_cur:
            mux_mode_prev = mux_mode_cur
            mux_mode_cur = mux_mode_temp
            tomo_mux_updated = True
            # print('tomo mux updated')
            continue  # skip this sample
        else:
            pass
            # print('tomo mux continued')

        if mode == 'd':
            # packet_number = int.from_bytes(buffer[2:4], byteorder='little')
            accumA1I = int.from_bytes(buffer[9:11], byteorder='little', signed="True")
            accumA1Q = int.from_bytes(buffer[11:13], byteorder='little', signed="True")
            accumB1I = int.from_bytes(buffer[13:15], byteorder='little', signed="True")
            accumB1Q = int.from_bytes(buffer[15:17], byteorder='little', signed="True")
            accumC1I = int.from_bytes(buffer[17:19], byteorder='little', signed="True")
            accumC1Q = int.from_bytes(buffer[19:21], byteorder='little', signed="True")

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

        accumD = int.from_bytes(buffer[21:23], byteorder='little', signed="False")
        ppg_red = int.from_bytes(buffer[23:25], byteorder='little', signed="False") & 0xFFFF

        magZ1 = abs(Z1)
        phZ1 = cmath.phase(Z1)
        magZ2 = abs(Z2)
        phZ2 = cmath.phase(Z2)

        # Add values to raw queues and process for downsampling
        ECG_data.add(accumD)
        PPG_RED_data.add(ppg_red)
        if op_mode == 0:
            # print('Wave mode')
            if mux_mode_cur == 1:
                if mux_mode_cur != mux_mode_prev:
                    mux_mode_prev = mux_mode_cur
                    # print('mode1')
                    # ser.read(packet_size * 4) # throw away some samples
                    # continue  # skip this sample
                BIOZ1_data.add(magZ1)
                BIOZ2_data.add(magZ2)
            elif mux_mode_cur == 2:
                if mux_mode_cur != mux_mode_prev:
                    mux_mode_prev = mux_mode_cur
                    # print('mode2')
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

        else:
            # print('Tomo mode')
            # exc_pin_index = ((mux_mode_cur >> 25) & 0x1F)
            # sns_ap_index = ((mux_mode_cur >> 20) & 0x1F)
            # print('from main', exc_pin_index, sns_ap_index)
            if tomo_mux_updated is True:
                Z1_TOMO_MAG_AVG = Z1_TOMO_MAG.average()
                Z2_TOMO_MAG_AVG = Z2_TOMO_MAG.average()
                Z1_TOMO_PHASE_AVG = Z1_TOMO_PHASE.average()
                Z2_TOMO_PHASE_AVG = Z2_TOMO_PHASE.average()
                Z1_TOMO_MAG.clear()
                Z2_TOMO_MAG.clear()
                Z1_TOMO_PHASE.clear()
                Z2_TOMO_PHASE.clear()

            Z1_TOMO_MAG.add(magZ1)
            # Z1_TOMO_MAG.add(Z1.real)
            Z1_TOMO_PHASE.add(phZ1)
            Z2_TOMO_MAG.add(magZ2)
            Z2_TOMO_PHASE.add(phZ2)

    else:
        print('Synchronization not confirmed. Use previous values')
        i = 0
        while not (buffer[i] == 0xC7 and buffer[(i + 1) % packet_size] == 0xC7):  # modify code, make unique sync. code
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
    elif freq.updated:
        freq.updated = False
        # ser.write(str(freq.curr).encode() if freq.curr < 10 else 'D'.encode())
        ser.write(b'f')
        ser.write(b'1')
        ser.write(str(freq.curr).encode())

        print(f'\n\n*******Excitation signal frequency is changed to {freq.get_str_label()}*******\n\n')
        # time.sleep(0.1)
        ser.reset_input_buffer()
        ser.read(packet_size * 500)  # dummy read
        continue
    if num_z_channels != num_z_channels_prev:
        num_z_channels_prev = num_z_channels
        ser.write(b'w')
        ser.write(b'1')
        ser.write({2: '2', 4: '4', 8: '8'}.get(num_z_channels).encode())
        plt.close('all')
        create_wave_figure()
        create_wave_buttons()

    if op_mode == 1 and tomo_first_time is True:
        plt.close('all')
        create_tomo_figure()
        create_tomo_buttons()
        tomo_first_time = False

    elif op_mode == 2 and tomo_first_time is True:
        plt.close('all')
        create_tomo2_figure()
        create_tomo_buttons()
        tomo_first_time = False

    if logFlag:
        if op_mode == 0:
            writer.writerow(
                [BIOZ1_data.raw[-1], BIOZ2_data.raw[-1], BIOZ3_data.raw[-1], BIOZ4_data.raw[-1], ECG_data.raw[-1],
                 PPG_RED_data.raw[-1],
                 str(freq.get_int_label()), mux_mode_cur, packet_number])

        # elif (tomo_mode is True) and (tomo_mux_updated is True):  # Only log 1 averaged value per mux position
        elif all((op_mode, tomo_mux_updated)):
            exc_pin = ((mux_mode_prev >> 25) & 0x1F) + 1
            sns_ap_pin = ((mux_mode_prev >> 20) & 0x1F) + 1
            sns_an_pin = ((mux_mode_prev >> 15) & 0x1F) + 1
            sns_bp_pin = ((mux_mode_prev >> 10) & 0x1F) + 1
            sns_bn_pin = ((mux_mode_prev >> 5) & 0x1F) + 1
            gnd_pin = (mux_mode_prev & 0x1F) + 1

            writer.writerow([round(Z1_TOMO_MAG_AVG, 6), round(Z1_TOMO_PHASE_AVG, 6), str(freq.get_int_label()),
                             exc_pin, gnd_pin, sns_ap_pin, sns_an_pin, packet_number])

        # elif tomo_mode is True:
        #    writer.writerow([magZ1])

    circular_counter += 1
    if (circular_counter >= 300) and op_mode == 0:
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

    # elif tomo_mode is 1 and tomo_mux_updated is True:
    elif all((op_mode == 1, tomo_mux_updated)):
        tomo_mux_updated = False
        circular_counter = 0

        exc_pin_index = ((mux_mode_prev >> 25) & 0x1F)
        sns_ap_index = ((mux_mode_prev >> 20) & 0x1F)
        # print(exc_pin_index, sns_ap_index, Z1_TOMO_MAG.counter, Z1_TOMO_MAG.averaged_values[Z1_TOMO_MAG.counter - 1])

        if not (exc_pin_index == 15 and sns_ap_index == 13):
            continue

        with warnings.catch_warnings():
            warnings.simplefilter("ignore", UserWarning)  # Ignore UserWarning
            lines[0].set_ydata(Z1_TOMO_MAG.averaged_values)
            axs[0].set_ylim(min(Z1_TOMO_MAG.averaged_values), max(Z1_TOMO_MAG.averaged_values))
            lines[1].set_ydata(Z1_TOMO_PHASE.averaged_values)
            axs[1].set_ylim(min(Z1_TOMO_PHASE.averaged_values), max(Z1_TOMO_PHASE.averaged_values))
            fig.canvas.draw_idle()
            plt.pause(0.1)

        # elif tomo_mode is 1 and tomo_mux_updated is True:
    elif all((op_mode == 2, tomo_mux_updated)):
        tomo_mux_updated = False
        circular_counter = 0

        exc_pin_index = ((mux_mode_prev >> 25) & 0x1F)
        sns_ap_index = ((mux_mode_prev >> 20) & 0x1F)
        # print(exc_pin_index, sns_ap_index, Z1_TOMO_MAG.counter, Z1_TOMO_MAG.averaged_values[Z1_TOMO_MAG.counter - 1])

        if not (exc_pin_index == 15 and sns_ap_index == 13):
            continue

        if Z1_TOMO_MAG.counter == 0:  # test for data validity
            reconstruct.update_plot(Z1_TOMO_MAG.averaged_values)

        else:
            print('Data not valid, skipping the frame')
        # reconstruct.update_plot(data_f)
        # print(Z1_TOMO_MAG.counter)
        Z1_TOMO_MAG.counter = 0     # force new loop



input('Press enter to exit')
