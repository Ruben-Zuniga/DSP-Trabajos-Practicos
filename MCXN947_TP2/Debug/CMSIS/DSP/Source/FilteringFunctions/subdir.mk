################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CMSIS/DSP/Source/FilteringFunctions/FilteringFunctions.c \
../CMSIS/DSP/Source/FilteringFunctions/FilteringFunctionsF16.c 

C_DEPS += \
./CMSIS/DSP/Source/FilteringFunctions/FilteringFunctions.d \
./CMSIS/DSP/Source/FilteringFunctions/FilteringFunctionsF16.d 

OBJS += \
./CMSIS/DSP/Source/FilteringFunctions/FilteringFunctions.o \
./CMSIS/DSP/Source/FilteringFunctions/FilteringFunctionsF16.o 


# Each subdirectory must supply rules for building sources it contributes
CMSIS/DSP/Source/FilteringFunctions/%.o: ../CMSIS/DSP/Source/FilteringFunctions/%.c CMSIS/DSP/Source/FilteringFunctions/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\workspace\MCXN947_TP2\drivers" -I"C:\workspace\MCXN947_TP2\device" -I"C:\workspace\MCXN947_TP2\utilities\debug_console" -I"C:\workspace\MCXN947_TP2\component\uart" -I"C:\workspace\MCXN947_TP2\utilities\debug_console\config" -I"C:\workspace\MCXN947_TP2\component\serial_manager" -I"C:\workspace\MCXN947_TP2\component\lists" -I"C:\workspace\MCXN947_TP2\device\periph" -I"C:\workspace\MCXN947_TP2\utilities" -I"C:\workspace\MCXN947_TP2\CMSIS" -I"C:\workspace\MCXN947_TP2\CMSIS\m-profile" -I"C:\workspace\MCXN947_TP2\utilities\str" -I"C:\workspace\MCXN947_TP2\CMSIS\DSP\Include" -I"C:\workspace\MCXN947_TP2\CMSIS\DSP\PrivateInclude" -I"C:\workspace\MCXN947_TP2\board" -I"C:\workspace\MCXN947_TP2\CMSIS\DSP\Source\DistanceFunctions" -I"C:\workspace\MCXN947_TP2\source" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-CMSIS-2f-DSP-2f-Source-2f-FilteringFunctions

clean-CMSIS-2f-DSP-2f-Source-2f-FilteringFunctions:
	-$(RM) ./CMSIS/DSP/Source/FilteringFunctions/FilteringFunctions.d ./CMSIS/DSP/Source/FilteringFunctions/FilteringFunctions.o ./CMSIS/DSP/Source/FilteringFunctions/FilteringFunctionsF16.d ./CMSIS/DSP/Source/FilteringFunctions/FilteringFunctionsF16.o

.PHONY: clean-CMSIS-2f-DSP-2f-Source-2f-FilteringFunctions

