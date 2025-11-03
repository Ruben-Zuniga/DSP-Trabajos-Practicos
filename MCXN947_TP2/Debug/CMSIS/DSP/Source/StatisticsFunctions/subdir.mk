################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctions.c \
../CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctionsF16.c 

C_DEPS += \
./CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctions.d \
./CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctionsF16.d 

OBJS += \
./CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctions.o \
./CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctionsF16.o 


# Each subdirectory must supply rules for building sources it contributes
CMSIS/DSP/Source/StatisticsFunctions/%.o: ../CMSIS/DSP/Source/StatisticsFunctions/%.c CMSIS/DSP/Source/StatisticsFunctions/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -DDISABLEFLOAT16 -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/drivers" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/device" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/utilities/debug_console" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/component/uart" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/utilities/debug_console/config" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/component/serial_manager" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/component/lists" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/device/periph" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/utilities" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/CMSIS" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/CMSIS/m-profile" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/utilities/str" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/CMSIS/DSP/Include" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/CMSIS/DSP/PrivateInclude" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/CMSIS/DSP/Source/DistanceFunctions" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/board" -I"/home/ruben/Documentos/Facultad/Procesamiento_Digital_de_Seniales/workspace2/MCXN947_TP1/source" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-CMSIS-2f-DSP-2f-Source-2f-StatisticsFunctions

clean-CMSIS-2f-DSP-2f-Source-2f-StatisticsFunctions:
	-$(RM) ./CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctions.d ./CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctions.o ./CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctionsF16.d ./CMSIS/DSP/Source/StatisticsFunctions/StatisticsFunctionsF16.o

.PHONY: clean-CMSIS-2f-DSP-2f-Source-2f-StatisticsFunctions

