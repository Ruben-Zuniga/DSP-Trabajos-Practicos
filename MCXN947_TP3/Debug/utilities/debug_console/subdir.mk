################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../utilities/debug_console/fsl_debug_console.c 

C_DEPS += \
./utilities/debug_console/fsl_debug_console.d 

OBJS += \
./utilities/debug_console/fsl_debug_console.o 


# Each subdirectory must supply rules for building sources it contributes
utilities/debug_console/%.o: ../utilities/debug_console/%.c utilities/debug_console/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -DSDK_DEBUGCONSOLE_UART -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"C:\workspace\MCXN947_TP3\drivers" -I"C:\workspace\MCXN947_TP3\device" -I"C:\workspace\MCXN947_TP3\utilities\debug_console" -I"C:\workspace\MCXN947_TP3\component\uart" -I"C:\workspace\MCXN947_TP3\utilities\debug_console\config" -I"C:\workspace\MCXN947_TP3\component\serial_manager" -I"C:\workspace\MCXN947_TP3\component\lists" -I"C:\workspace\MCXN947_TP3\device\periph" -I"C:\workspace\MCXN947_TP3\utilities" -I"C:\workspace\MCXN947_TP3\CMSIS" -I"C:\workspace\MCXN947_TP3\CMSIS\m-profile" -I"C:\workspace\MCXN947_TP3\utilities\str" -I"C:\workspace\MCXN947_TP3\CMSIS\DSP\Include" -I"C:\workspace\MCXN947_TP3\CMSIS\DSP\PrivateInclude" -I"C:\workspace\MCXN947_TP3\board" -I"C:\workspace\MCXN947_TP3\source" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-utilities-2f-debug_console

clean-utilities-2f-debug_console:
	-$(RM) ./utilities/debug_console/fsl_debug_console.d ./utilities/debug_console/fsl_debug_console.o

.PHONY: clean-utilities-2f-debug_console

