################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-822241249: ../c2000.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs2031/ccs/utils/sysconfig_1.25.0/sysconfig_cli.bat" --script "//VMWARE-HOST/Shared Folders/D/new_z_box/dsp-impedance/NEW_Z_BOX/new_dsp_z/c2000.syscfg" -o "syscfg" -s "C:/ti/C2000Ware_6_00_00_00/.metadata/sdk.json" -d "F2837xD" --compiler ccs
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/error.h: build-822241249 ../c2000.syscfg
syscfg: build-822241249

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs2031/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -Ooff --include_path="//VMWARE-HOST/Shared Folders/D/new_z_box/dsp-impedance/NEW_Z_BOX/new_dsp_z" --include_path="//VMWARE-HOST/Shared Folders/D/new_z_box/dsp-impedance/NEW_Z_BOX/new_dsp_z/device" --include_path="C:/ti/C2000Ware_6_00_00_00/driverlib/f2837xd/driverlib/" --include_path="C:/ti/ccs2031/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS/include" --define=DEBUG --define=CPU1 --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="//VMWARE-HOST/Shared Folders/D/new_z_box/dsp-impedance/NEW_Z_BOX/new_dsp_z/CPU1_RAM/syscfg" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


