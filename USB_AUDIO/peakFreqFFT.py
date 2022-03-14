import pyaudio
import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
from matplotlib.widgets import CheckButtons

filter_50_mode = 0
filter_100_mode = 0


np.set_printoptions(suppress=True) # don't use scientific notation

CHUNK = 8192 # number of data points to read at a time
RATE = 44100 # time resolution of the recording device (Hz)

p=pyaudio.PyAudio() # start the PyAudio class
stream=p.open(format=pyaudio.paInt16,channels=1,rate=RATE,input=True,
              frames_per_buffer=CHUNK) #uses default input device
              
#Design notch filters
fs = 44100.0  # Sample frequency (Hz)
f1 = 50.0  # Frequency to be removed from signal (Hz)
f2 = 100.0  # Frequency to be removed from signal (Hz)
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
                 
plt.figure(figsize=(8,6), dpi=100)
axA = plt.axes([0.1, 0.40, 0.8, 0.55])
axA.plot(1)

ax1 = plt.axes([0.1, 0.15, 0.1, 0.1])
labels = ['50Hz','100Hz']
b1 = CheckButtons(ax1, labels)
b1.on_clicked(filters)

# create a numpy array holding a single read of audio data
while True: #to it a few times just to see
    data = np.frombuffer(stream.read(CHUNK),dtype=np.int16)
    if (filter_50_mode):
        data = signal.filtfilt(b50, a50, data) #50Hz remove
    if (filter_100_mode):
        data = signal.filtfilt(b100, a100, data) #100Hz remove
    data = data * np.hanning(len(data)) # smooth the FFT by windowing data
    fft = abs(np.fft.fft(data).real)
    fft = abs(np.fft.fft(data))
    fft = fft[:int(len(fft)/2)] # keep only first half
    freq = np.fft.fftfreq(CHUNK,1.0/RATE)
    freq = freq[:int(len(freq)/2)] # keep only first half
    freqPeak = freq[np.where(fft==np.max(fft))[0][0]]+1
    print("peak frequency: %d Hz"%freqPeak)

    # uncomment this if you want to see what the freq vs FFT looks like       
    axA.cla()
    axA.plot(freq,fft)
    axA.axis([0,4000,None,None])
    plt.draw()
    plt.pause(0.01)
    
    

# close the stream gracefully
stream.stop_stream()
stream.close()
p.terminate()