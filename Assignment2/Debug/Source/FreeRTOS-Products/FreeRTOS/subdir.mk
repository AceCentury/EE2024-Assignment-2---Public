################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Source/FreeRTOS-Products/FreeRTOS/croutine.c \
../Source/FreeRTOS-Products/FreeRTOS/list.c \
../Source/FreeRTOS-Products/FreeRTOS/queue.c \
../Source/FreeRTOS-Products/FreeRTOS/tasks.c \
../Source/FreeRTOS-Products/FreeRTOS/timers.c 

OBJS += \
./Source/FreeRTOS-Products/FreeRTOS/croutine.o \
./Source/FreeRTOS-Products/FreeRTOS/list.o \
./Source/FreeRTOS-Products/FreeRTOS/queue.o \
./Source/FreeRTOS-Products/FreeRTOS/tasks.o \
./Source/FreeRTOS-Products/FreeRTOS/timers.o 

C_DEPS += \
./Source/FreeRTOS-Products/FreeRTOS/croutine.d \
./Source/FreeRTOS-Products/FreeRTOS/list.d \
./Source/FreeRTOS-Products/FreeRTOS/queue.d \
./Source/FreeRTOS-Products/FreeRTOS/tasks.d \
./Source/FreeRTOS-Products/FreeRTOS/timers.d 


# Each subdirectory must supply rules for building sources it contributes
Source/FreeRTOS-Products/FreeRTOS/%.o: ../Source/FreeRTOS-Products/FreeRTOS/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -D__USE_CMSIS=CMSISv2p00_LPC17xx -I"C:\Users\Ryan\Desktop\Test Space\Lib_EaBaseBoard\inc" -I"C:\Users\Ryan\Desktop\Test Space\Assignment2\Source\Helper_Functions" -I"C:\Users\Ryan\Desktop\Test Space\Assignment2\Source\Include" -I"C:\Users\Ryan\Desktop\Test Space\Assignment2\Source\FreeRTOS-Products\FreeRTOS\portable\GCC\ARM_CM3" -I"C:\Users\Ryan\Desktop\Test Space\Assignment2\Source\FreeRTOS-Products\FreeRTOS\include" -I"C:\Users\Ryan\Desktop\Test Space\CMSISv2p00_LPC17xx\inc" -I"C:\Users\Ryan\Desktop\Test Space\Assignment2\Source" -I"C:\Users\Ryan\Desktop\Test Space\lpc17xx.cmsis.driver.library\Include" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -Wextra -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


