How to operate ICG, ECG, PPG measurement device:

Connect PPG sensor (It is important to connect PPG before powering device up, because device
programs PPG only once, when it is powered up)
Install batteries (not working over USB power)
Turn on battery power switch
Connect USB to your computer. (In this point you need to have a driver for XDS100V2 USB JTAG
and for this you have to install Code Composer Studio)
Now, you can open GUI. There are two alternatives (different file format) for the same GUI , one
is executable (GUI_Vxx.exe) and the other is Python script (GUI_Vxx.py). You can run executable
without any installations, in Windows machines.
(Optional) If you want to run python script instead, you must install python and dependencies
(pyserial, keyboard, scipy, matplotlib). After installing Python, you can use “pip install pyserial”
command on Windows terminal to install pyserial module, for example (also same for other
dependencies)
When GUI is opened you will see similar screen below. Sometimes GUI does not open in the first
try, in this case try once again.



• 200Hz, 2kHz, 20kHz buttons set excitation frequency for ICG measurement. 20kHz is
default
• Decimal, Floating is calculation mode for ICG signals, default is Floating
• 50Hz, 100Hz check buttons apply band stop filters to ICG and ECG signals.
