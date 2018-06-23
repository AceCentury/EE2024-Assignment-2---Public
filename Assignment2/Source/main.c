/* EE2024 AY17/18 Assignment 2
 * Aaron Soh Yu Han
 * Teo Meng Shin, Ryan
 */

/* Standard includes. */
#include <string.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Library includes. */
#include "LPC17xx.h"

// Helper Includes
#include "init_interfaces.h"
#include "create_tasks.h"

#include "lpc17xx_gpio.h"
#include "lpc17xx_uart.h"
#include "my_uart.h"

system_mode_t CURRENT_MODE = STATIONARY; // 0 for STATIONARY, 1 FOR LAUNCH, 2 FOR RETURN
volatile uint32_t MODE_TOGGLE_PRESSES = 0;
volatile portTickType LAST_TOGGLE_PRESS = 0;
volatile portTickType CURR_TOGGLE_PRESS;

volatile warning_state TEMP_WARNING = OFF; // OFF for no warning, ON for warning
static const int DEFAULT_TEMP_THRESHOLD = 28;
volatile int TEMP_WARNING_THRESHOLD = 28;

volatile warning_state ACC_WARNING = OFF; //0 for not warning, 1 for warning
static const float DEFAULT_ACC_THRESHOLD = 0.4;
volatile float ACC_THRESHOLD = 0.4; //Units in G's

static const uint32_t DEFAULT_OBSTACLE_THRESHOLD = 3000;
volatile uint32_t OBSTACLE_NEAR_THRESHOLD = 3000;

uint8_t *TEMP_WARNING_TEXT = "Temps. Too High!";
uint8_t *TEMP_SAFE_TEXT = "Temps. Safe!";

// List of task handles
xTaskHandle enter_stationary_handle;
xTaskHandle temperature_stationary_handle;
xTaskHandle countdown_handle;
xTaskHandle temp_warning_stationary_handle;
xTaskHandle begin_countdown_handle;

xTaskHandle blink_red_handle;

xTaskHandle enter_launch_handle;
xTaskHandle temp_warning_launch_handle;
xTaskHandle accel_warning_handle;
xTaskHandle switch_to_return_handle;
xTaskHandle launch_mode_monitoring_handle;

xTaskHandle blink_blue_handle;
xTaskHandle blink_blue_and_red_handle;

xTaskHandle enter_return_handle;
xTaskHandle light_sensor_handle;
xTaskHandle switch_to_stationary_handle;

xTaskHandle joystick_handle;
xTaskHandle send_NUSCloud_handle;

volatile int32_t CURR_TEMP = 0;
volatile int32_t temp_counter = 0;
volatile int8_t curr_temp_state;
volatile int8_t last_temp_state;
volatile portTickType t1 = 0;
volatile portTickType t2 = 0;

volatile uint8_t JOYSTICK_MODE = 0;

void EINT3_IRQHandler(void) {
	//Temp sensor is at P0.2
	if (((LPC_GPIOINT ->IO0IntStatF >> 2) & 0x1)
			|| ((LPC_GPIOINT ->IO0IntStatR >> 2) & 0x1)) {
		curr_temp_state = ((GPIO_ReadValue(0) & (1 << 2)) != 0);
		if (last_temp_state == curr_temp_state)
			LPC_GPIOINT ->IO0IntClr = 1 << 2;
		else {
			if (temp_counter == 0)
				t1 = xTaskGetTickCountFromISR();
			temp_counter++;
			last_temp_state = curr_temp_state;
		}

		if (temp_counter == 340) {
			t2 = xTaskGetTickCountFromISR();
			if (t2 > t1) {
				t2 = t2 - t1;
			} else {
				t2 = (0xFFFFFFFF - t1 + 1) + t2;
			}
			temp_counter = 0;
			CURR_TEMP = ((2 * 1000 * t2) / (340 * 1) - 2731); //Formula taken from temp.c
		}

		LPC_GPIOINT ->IO0IntClr = 1 << 2;
	}

	//Check P2.4 Joystick Left
	else if (((LPC_GPIOINT ->IO2IntStatF >> 4) & 0x1)) {
		if (JOYSTICK_MODE == 0) {
			JOYSTICK_MODE = 2;
		}

		else
			JOYSTICK_MODE = (JOYSTICK_MODE - 1);
		xTaskResumeFromISR(joystick_handle);
		LPC_GPIOINT ->IO2IntClr = 1 << 4;
	}

	//Check P0.16 Joystick Right
	else if (((LPC_GPIOINT ->IO0IntStatF >> 16) & 0x1)) {
		JOYSTICK_MODE = (JOYSTICK_MODE + 1) % 3;
		xTaskResumeFromISR(joystick_handle);
		LPC_GPIOINT ->IO0IntClr = 1 << 16;
	}

	//Check P2.3 Joystick Up
	else if (((LPC_GPIOINT ->IO2IntStatF >> 3) & 0x1)) {
		if (JOYSTICK_MODE == 0) {
			if (TEMP_WARNING_THRESHOLD < 35)
				TEMP_WARNING_THRESHOLD++;
		}

		else if (JOYSTICK_MODE == 1) {
			if (ACC_THRESHOLD < 0.8)
				ACC_THRESHOLD += 0.05;
		}

		else if (JOYSTICK_MODE == 2) {
			if (OBSTACLE_NEAR_THRESHOLD < 3500)
				OBSTACLE_NEAR_THRESHOLD += 100;
		}
		xTaskResumeFromISR(joystick_handle);
		LPC_GPIOINT ->IO2IntClr = 1 << 3;
	}

	//Check P0.15 Joystick Down
	else if (((LPC_GPIOINT ->IO0IntStatF >> 15) & 0x1)) {
		if (JOYSTICK_MODE == 0) {
			if (TEMP_WARNING_THRESHOLD > 20)
				TEMP_WARNING_THRESHOLD--;
		}

		else if (JOYSTICK_MODE == 1) {
			if (ACC_THRESHOLD > 0.2)
				ACC_THRESHOLD -= 0.05;
		}

		else if (JOYSTICK_MODE == 2) {
			if (OBSTACLE_NEAR_THRESHOLD > 2500)
				OBSTACLE_NEAR_THRESHOLD -= 100;
		}
		xTaskResumeFromISR(joystick_handle);
		LPC_GPIOINT ->IO0IntClr = 1 << 15;
	}

	//Check P0.17 Joystick Center reset thresholds
	else if (((LPC_GPIOINT ->IO0IntStatF >> 17) & 0x1)) {
		TEMP_WARNING_THRESHOLD = DEFAULT_TEMP_THRESHOLD;
		ACC_THRESHOLD = DEFAULT_ACC_THRESHOLD;
		OBSTACLE_NEAR_THRESHOLD = DEFAULT_OBSTACLE_THRESHOLD;
		xTaskResumeFromISR(joystick_handle);
		LPC_GPIOINT ->IO0IntClr = 1 << 17;
	}

}

void EINT0_IRQHandler(void) {
	CURR_TOGGLE_PRESS = xTaskGetTickCountFromISR();

	if (abs(CURR_TOGGLE_PRESS - LAST_TOGGLE_PRESS) > 50) { //Debouncer
		MODE_TOGGLE_PRESSES++;
		if (CURRENT_MODE == STATIONARY) { //In stationary mode, begin countdown
			xTaskResumeFromISR(begin_countdown_handle);
		} else if (CURRENT_MODE == LAUNCH) {
			xTaskResumeFromISR(switch_to_return_handle);
		} else if (CURRENT_MODE == RETURN) {
			xTaskResumeFromISR(switch_to_stationary_handle);
		}
	}
	LAST_TOGGLE_PRESS = CURR_TOGGLE_PRESS;
	LPC_SC ->EXTINT = (1 << 0); //Clearing interrupt of EINT0
}

//UART3 interrupt handler
void UART3_IRQHandler(void) {
	UART3_StdIntHandler();
}

int main(void) {
	/* The examples assume that all priority bits are assigned as preemption
	 priority bits. */
	NVIC_SetPriorityGrouping(0UL);

	setup_interfaces();

	create_all_tasks();

	// Enable and clear GPIO Interrupt P0.2 for Temp Sensor.
	GPIO_SetDir(0, (1 << 2), 0);
	LPC_GPIOINT ->IO0IntEnF |= 1 << 2;
	LPC_GPIOINT ->IO0IntEnR |= 1 << 2;
	LPC_GPIOINT ->IO0IntClr = 1 << 2;
	curr_temp_state = ((GPIO_ReadValue(0) & (1 << 2)) != 0);
	last_temp_state = curr_temp_state;

	//Configure EINT0 - pin was configured in setup interfaces
	LPC_SC ->EXTMODE = (1 << 0); //Edge detection
	LPC_SC ->EXTPOLAR = (0 << 0); //Falling Edge
	LPC_SC ->EXTINT = (0 << 0); //Clearing interrupt of EINT0

	//Clear Pending Status
	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_ClearPendingIRQ(EINT3_IRQn);
	NVIC_ClearPendingIRQ(UART3_IRQn);

	//Setting priorities - lower number = higher priority
	NVIC_SetPriority(EINT3_IRQn, 2); //Temp sensor and Joystick
	NVIC_SetPriority(EINT0_IRQn, 1); //MODE_TOGGLE
	NVIC_SetPriority(UART3_IRQn, 0); //UART

// Enable interrupts - UART3 was enabled inside setup interfaces
	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT3_IRQn);

	if (SysTick_Config(SystemCoreClock / 1000)) {
		while (1)
			;
	}

	vTaskStartScheduler();

	return 0;
}

void check_failed(uint8_t *file, uint32_t line) {
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
		;
}

/*-----------------------------------------------------------*/

/* Used in the run time stats calculations. */
static uint32_t ulClocksPer10thOfAMilliSecond = 0UL;

/*-----------------------------------------------------------*/

void vMainConfigureTimerForRunTimeStats(void) {
	/* How many clocks are there per tenth of a millisecond? */
	ulClocksPer10thOfAMilliSecond = configCPU_CLOCK_HZ / 10000UL;
}
/*-----------------------------------------------------------*/

uint32_t ulMainGetRunTimeCounterValue(void) {
	uint32_t ulSysTickCounts, ulTickCount, ulReturn;
	const uint32_t ulSysTickReloadValue = (configCPU_CLOCK_HZ
			/ configTICK_RATE_HZ ) - 1UL;
	volatile uint32_t * const pulCurrentSysTickCount =
			((volatile uint32_t *) 0xe000e018);
	volatile uint32_t * const pulInterruptCTRLState =
			((volatile uint32_t *) 0xe000ed04);
	const uint32_t ulSysTickPendingBit = 0x04000000UL;

	/* NOTE: There are potentially race conditions here.  However, it is used
	 anyway to keep the examples simple, and to avoid reliance on a separate
	 timer peripheral. */

	/* The SysTick is a down counter.  How many clocks have passed since it was
	 last reloaded? */
	ulSysTickCounts = ulSysTickReloadValue - *pulCurrentSysTickCount;

	/* How many times has it overflowed? */
	ulTickCount = xTaskGetTickCountFromISR();

	/* Is there a SysTick interrupt pending? */
	if ((*pulInterruptCTRLState & ulSysTickPendingBit) != 0UL) {
		/* There is a SysTick interrupt pending, so the SysTick has overflowed
		 but the tick count not yet incremented. */
		ulTickCount++;

		/* Read the SysTick again, as the overflow might have occurred since
		 it was read last. */
		ulSysTickCounts = ulSysTickReloadValue - *pulCurrentSysTickCount;
	}

	/* Convert the tick count into tenths of a millisecond.  THIS ASSUMES
	 configTICK_RATE_HZ is 1000! */
	ulReturn = (ulTickCount * 10UL);

	/* Add on the number of tenths of a millisecond that have passed since the
	 tick count last got updated. */
	ulReturn += (ulSysTickCounts / ulClocksPer10thOfAMilliSecond);

	return ulReturn;
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName) {
	(void) pcTaskName;
	(void) pxTask;

	/* Run time stack overflow checking is performed if
	 configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	 function is called if a stack overflow is detected. */taskDISABLE_INTERRUPTS();
	for (;;)
		;
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void) {
	/* vApplicationMallocFailedHook() will only be called if
	 configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	 function that will get called if a call to pvPortMalloc() fails.
	 pvPortMalloc() is called internally by the kernel whenever a task, queue,
	 timer or semaphore is created.  It is also called by various parts of the
	 demo application.  If heap_1.c or heap_2.c are used, then the size of the
	 heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	 FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	 to query the size of free heap space that remains (although it does not
	 provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
	for (;;)
		;
}
/*-----------------------------------------------------------*/

