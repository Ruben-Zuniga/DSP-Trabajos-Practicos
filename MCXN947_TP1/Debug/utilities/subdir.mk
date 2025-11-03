################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../utilities/fsl_assert.c 

C_DEPS += \
./utilities/fsl_assert.d 

OBJS += \
./utilities/fsl_assert.o 


# Each subdirectory must supply rules for building sources it contributes
utilities/%.o: ../utilities/%.c utilities/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -DDISABLEFLOAT16 -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/drivers" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/device" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/utilities/debug_console" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/component/uart" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/utilities/debug_console/config" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/component/serial_manager" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/component/lists" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/device/periph" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/utilities" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/CMSIS" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/CMSIS/m-profile" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/utilities/str" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/CMSIS/DSP/Include" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/CMSIS/DSP/PrivateInclude" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/CMSIS/DSP/Source/DistanceFunctions" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/board" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/source" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-utilities

clean-utilities:
	-$(RM) ./utilities/fsl_assert.d ./utilities/fsl_assert.o

.PHONY: clean-utilities

