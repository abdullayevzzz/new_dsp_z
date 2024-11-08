################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
other_includes/%.obj: ../other_includes/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs1210/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -Ooff --include_path="C:/Users/admin/workspace_v12/Blue_ECG_ICG_PPG_V7_cpu01" --include_path="C:/Users/admin/workspace_v12/Blue_ECG_ICG_PPG_V7_cpu01/device" --include_path="C:/ti/c2000/C2000Ware_4_01_00_00/driverlib/f2837xd/driverlib" --include_path="C:/ti/c2000/C2000Ware_4_01_00_00/libraries/calibration/hrpwm/F2837xD/include" --include_path="C:/ti/c2000/C2000Ware_4_01_00_00/device_support/f2837xd/headers/include" --include_path="C:/ti/c2000/C2000Ware_4_01_00_00/device_support/f2837xd/common/include" --include_path="C:/ti/ccs1210/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS/include" --define=_LAUNCHXL_F28379D --define=DEBUG --define=CPU1 --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="other_includes/$(basename $(<F)).d_raw" --include_path="C:/Users/admin/workspace_v12/Blue_ECG_ICG_PPG_V7_cpu01/CPU1_RAM/syscfg" --obj_directory="other_includes" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

other_includes/%.obj: ../other_includes/%.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs1210/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -Ooff --include_path="C:/Users/admin/workspace_v12/Blue_ECG_ICG_PPG_V7_cpu01" --include_path="C:/Users/admin/workspace_v12/Blue_ECG_ICG_PPG_V7_cpu01/device" --include_path="C:/ti/c2000/C2000Ware_4_01_00_00/driverlib/f2837xd/driverlib" --include_path="C:/ti/c2000/C2000Ware_4_01_00_00/libraries/calibration/hrpwm/F2837xD/include" --include_path="C:/ti/c2000/C2000Ware_4_01_00_00/device_support/f2837xd/headers/include" --include_path="C:/ti/c2000/C2000Ware_4_01_00_00/device_support/f2837xd/common/include" --include_path="C:/ti/ccs1210/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS/include" --define=_LAUNCHXL_F28379D --define=DEBUG --define=CPU1 --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="other_includes/$(basename $(<F)).d_raw" --include_path="C:/Users/admin/workspace_v12/Blue_ECG_ICG_PPG_V7_cpu01/CPU1_RAM/syscfg" --obj_directory="other_includes" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


