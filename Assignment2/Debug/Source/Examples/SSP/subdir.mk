################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Source/Examples/SSP/SPI-writes-to-7-seg-display.c 

OBJS += \
./Source/Examples/SSP/SPI-writes-to-7-seg-display.o 

C_DEPS += \
./Source/Examples/SSP/SPI-writes-to-7-seg-display.d 


# Each subdirectory must supply rules for building sources it contributes
Source/Examples/SSP/%.o: ../Source/Examples/SSP/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -D__USE_CMSIS=CMSISv2p00_LPC17xx -I"C:\Users\Ryan\OneDrive\EE2024\Lab\FreeRTOS Test\FreeRTOS-Products\FreeRTOS-Plus-IO\Device\LPC17xx\SupportedBoards" -I"C:\Users\Ryan\OneDrive\EE2024\Lab\FreeRTOS Test\FreeRTOS-Products\FreeRTOS-Plus-IO\Include" -I"C:\Users\Ryan\OneDrive\EE2024\Lab\FreeRTOS Test\FreeRTOS-Products\FreeRTOS-Plus-CLI" -I"C:\Users\Ryan\OneDrive\EE2024\Lab\FreeRTOS Test\FreeRTOS-Products\FreeRTOS\portable\GCC\ARM_CM3" -I"C:\Users\Ryan\OneDrive\EE2024\Lab\FreeRTOS Test\FreeRTOS-Products\FreeRTOS\include" -I"C:\Users\Ryan\OneDrive\EE2024\Lab\FreeRTOS Test\CMSISv2p00_LPC17xx\inc" -I"C:\Users\Ryan\OneDrive\EE2024\Lab\FreeRTOS Test\FreeRTOS-Plus-Demo-1\Source\Examples\Include" -I"C:\Users\Ryan\OneDrive\EE2024\Lab\FreeRTOS Test\FreeRTOS-Plus-Demo-1\Source" -I"C:\Users\Ryan\OneDrive\EE2024\Lab\FreeRTOS Test\lpc17xx.cmsis.driver.library\Include" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -Wextra -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


