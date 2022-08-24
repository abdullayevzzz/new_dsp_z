Steps to run DSP code after installing Code Composer Studio and C2000Ware

1. Open Resource Explorer at Code Composer Studio and navigate to this folder:
/Sofrware/C2000Ware(x.xx.xx.xx)/English/Devices/F2837XD/F28379D/Examples/Driverlib/CPU1/adc_ex11_multiple_soc_epwm   and import the project

2. In the project explorer window, right click to the imported project and choose properties. In the opened window, open Build -> C2000 Compiler -> Predefined Symbols.  Add this phrase to the Pre-define NAME section:   _LAUNCHXL_F28379D

3. Copy and paste all files in the latest project folder in this repository to the local project folder and replace the files with the same name.

4. Click Flash button to download the code to the DSP

5. If you have problem with target configuration, in the left side of the screen, in project explorer tab, click the current project, and under targetCongifs file there must be "TMS320F28377D.ccxml" file. Right click this file and click "Set As Active Target Configuration".


