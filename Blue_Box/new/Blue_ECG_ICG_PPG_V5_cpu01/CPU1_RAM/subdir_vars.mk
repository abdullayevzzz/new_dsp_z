################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CMD_SRCS += \
../2837xD_RAM_lnk_cpu1.cmd 

LIB_SRCS += \
C:/ti/c2000/C2000Ware_4_01_00_00/libraries/calibration/hrpwm/f2837xd/lib/SFO_v8_fpu_lib_build_c28_driverlib.lib \
C:/ti/c2000/C2000Ware_4_01_00_00/driverlib/f2837xd/driverlib/ccs/Debug/driverlib.lib 

ASM_SRCS += \
../dmac1.asm 

C_SRCS += \
../data_packer.c \
../exc_signal_dma.c \
../funcs.c \
../main.c \
../sci_b_esp32.c \
../wave_gen.c 

C_DEPS += \
./data_packer.d \
./exc_signal_dma.d \
./funcs.d \
./main.d \
./sci_b_esp32.d \
./wave_gen.d 

OBJS += \
./data_packer.obj \
./dmac1.obj \
./exc_signal_dma.obj \
./funcs.obj \
./main.obj \
./sci_b_esp32.obj \
./wave_gen.obj 

ASM_DEPS += \
./dmac1.d 

OBJS__QUOTED += \
"data_packer.obj" \
"dmac1.obj" \
"exc_signal_dma.obj" \
"funcs.obj" \
"main.obj" \
"sci_b_esp32.obj" \
"wave_gen.obj" 

C_DEPS__QUOTED += \
"data_packer.d" \
"exc_signal_dma.d" \
"funcs.d" \
"main.d" \
"sci_b_esp32.d" \
"wave_gen.d" 

ASM_DEPS__QUOTED += \
"dmac1.d" 

C_SRCS__QUOTED += \
"../data_packer.c" \
"../exc_signal_dma.c" \
"../funcs.c" \
"../main.c" \
"../sci_b_esp32.c" \
"../wave_gen.c" 

ASM_SRCS__QUOTED += \
"../dmac1.asm" 


