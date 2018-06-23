#include "launch_tasks.h"

#include <stdio.h>

//FreeRTOS Includes
#include "FreeRTOS.h"
#include "task.h"

//LPC Board Includes
#include "lpc17xx_gpio.h"

#include "oled.h"

//Helper Includes
#include "oled_helpers.h"
#include "my_rgb.h"
#include "acc.h"
#include "led7seg.h"
#include "init_interfaces.h"

extern system_mode_t CURRENT_MODE;
extern int MODE_TOGGLE_PRESSES;
extern warning_state TEMP_WARNING;
extern int TEMP_WARNING_THRESHOLD;

// Declaring variables
uint8_t *LAUNCH_TEXT = "LAUNCH";
uint8_t *ACCEL_WARNING_TEXT = "Veer Off Course";
uint8_t *ACCEL_SAFE_TEXT = "Course Safe!";
unsigned char command[15] = "";
unsigned char xReading[20] = "";
unsigned char yReading[20] = "";
float xG = 0;
float yG = 0;
extern uint8_t *TEMP_WARNING_TEXT;
extern uint8_t *TEMP_SAFE_TEXT;
extern int32_t CURR_TEMP;
extern char msg[20];
extern unsigned char tempReading[15];

extern warning_state ACC_WARNING;
extern float ACC_THRESHOLD;

extern xTaskHandle temperature_stationary_handle;
extern xTaskHandle countdown_handle;
extern xTaskHandle temp_warning_stationary_handle;
extern xTaskHandle begin_countdown_handle;

extern xTaskHandle enter_launch_handle;
extern xTaskHandle temperature_launch_handle;
extern xTaskHandle temp_warning_launch_handle;
extern xTaskHandle accelerometer_handle;
extern xTaskHandle accel_warning_handle;
extern xTaskHandle launch_mode_monitoring_handle;

extern xTaskHandle blink_blue_handle;
extern xTaskHandle blink_red_handle;
extern xTaskHandle blink_blue_and_red_handle;
extern xTaskHandle enter_return_handle;

extern xTaskHandle joystick_handle;

void enter_launch(void *ptr) {
	for (;;) {
		vTaskSuspend(NULL );
		sprintf(msg, "Entering LAUNCH mode \r\n");
		UART_SendString(LPC_UART3, (uint8_t *) msg);
		vTaskSuspend(temperature_stationary_handle);
		vTaskSuspend(countdown_handle);
		vTaskSuspend(temp_warning_stationary_handle);
		vTaskSuspend(blink_red_handle);
		vTaskSuspend(begin_countdown_handle);
		//Clear Warnings
		RGB_RED_OFF();
		TEMP_WARNING = OFF;
		ACC_WARNING = OFF;
		CURRENT_MODE = LAUNCH; //Set LAUNCH mode
		oled_clearScreen(OLED_COLOR_BLACK);
		printLineOled(LAUNCH_TEXT, 0);
		printLineOled(TEMP_SAFE_TEXT, 2);
		printLineOled(ACCEL_SAFE_TEXT, 5);
		led7seg_setChar('0', FALSE);
		vTaskResume(joystick_handle);
		vTaskResume(launch_mode_monitoring_handle);
	}
}

void temp_warning_launch(void *ptr) {
	for (;;) {
		vTaskSuspend(NULL );
		printLineOled(TEMP_WARNING_TEXT, 2);
		if (ACC_WARNING == ON) {
			vTaskSuspend(blink_blue_handle);
			RGB_BLUE_OFF();
			vTaskResume(blink_blue_and_red_handle);
		} else {
			vTaskResume(blink_red_handle);
		}
		sprintf(msg, "Temp Too High!\r\n");
		UART_SendString(LPC_UART3, (uint8_t *) msg);
	}
}

void accel_warning(void *ptr) {
	for (;;) {
		vTaskSuspend(NULL );
		printLineOled(ACCEL_WARNING_TEXT, 5);
		if (TEMP_WARNING == ON) {
			vTaskSuspend(blink_red_handle);
			RGB_RED_OFF();
			vTaskResume(blink_blue_and_red_handle);
		}

		else {
			vTaskResume(blink_blue_handle);
		}
		sprintf(msg, "Veer Off Course!\r\n");
		UART_SendString(LPC_UART3, (uint8_t *) msg);
	}
}

void blink_blue(void *ptr) {
	vTaskSuspend(NULL );
	portTickType xLastWakeTime;
	const portTickType xFrequency = 333;
	for (;;) {
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		RGB_BLUE_ON();
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		RGB_BLUE_OFF();
	}
}

void clear_warnings() {
	vTaskSuspend(blink_red_handle);
	vTaskSuspend(blink_blue_handle);
	vTaskSuspend(blink_blue_and_red_handle);
	RGB_RED_OFF();
	RGB_BLUE_OFF();
	clearLineOled(2);
	printLineOled(TEMP_SAFE_TEXT, 2);
	clearLineOled(5);
	printLineOled(ACCEL_SAFE_TEXT, 5);
	TEMP_WARNING = OFF;
	ACC_WARNING = OFF;
}

void blink_blue_and_red(void *ptr) {
	vTaskSuspend(NULL );
	portTickType xLastWakeTime;
	const portTickType xFrequency = 333;
	for (;;) {
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		RGB_BLUE_ON();
		RGB_RED_OFF();
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		RGB_BLUE_OFF();
		RGB_RED_ON();
	}
}

void switch_to_return(void *ptr) {
	int last_press_count;
	int curr_press_count;
	int count;
	portTickType xLastWakeTime;
	const portTickType xFrequency = 1000;
	vTaskSuspend(NULL );
	for (;;) {
		last_press_count = MODE_TOGGLE_PRESSES;
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		curr_press_count = MODE_TOGGLE_PRESSES;
		count = curr_press_count - last_press_count;
		if (count == 1) { //Set to detect 1 press because the first press would unblock this task.
			vTaskResume(enter_return_handle);
		}
		MODE_TOGGLE_PRESSES = 0; //Prevent overflow
		vTaskSuspend(NULL );
	}
}

void launch_mode_monitoring(void *ptr) { //MONITORING TEMP and ACCEL
	vTaskSuspend(NULL );
	int8_t SW4;
	//Accelerometer vars
	int8_t x = 0;
	int8_t y = 0;
	int8_t z = 0;

	//Temperature vars
	float temperature = 0;
	temperature = (CURR_TEMP / 10.0);
	sprintf(tempReading, "Temp = %.2f \r\n", temperature);
	printLineOled(tempReading, 1);
	portTickType xLastWakeTime;
	const portTickType xFrequency = 70;
	for (;;) {
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		if (TEMP_WARNING == ON || ACC_WARNING == ON) {
			//Read SW4 for CLEAR_WARNING
			SW4 = (GPIO_ReadValue(1) >> 31) & 1;
			if ((SW4 == 0)) {
				clear_warnings();
			}
		}
		//Updating temperature on OLED
		temperature = (CURR_TEMP / 10.0);
		if ((temperature > TEMP_WARNING_THRESHOLD) && TEMP_WARNING == OFF) { //Trigger Warning Sequence
			TEMP_WARNING = ON;
			vTaskResume(temp_warning_launch_handle);
		}
		sprintf(tempReading, "Temp = %.2f \r\n", temperature);
		//Updating and checking accelerometer
		acc_read(&x, &y, &z);
		// Converting to "g's"
		xG = x / 64.0;
		yG = y / 64.0;
		sprintf(xReading, "ACC X = %.2f \r\n", xG);
		sprintf(yReading, "ACC Y = %.2f \r\n", yG);
		printLineOled(xReading, 3);
		printLineOled(yReading, 4);
		printLineOled(tempReading, 1);
		if (ACC_WARNING == OFF
				&& (xG > ACC_THRESHOLD || yG > ACC_THRESHOLD
						|| xG < -ACC_THRESHOLD || yG < -ACC_THRESHOLD)) { //X or Y Threshold exceeded
			ACC_WARNING = ON;
			vTaskResume(accel_warning_handle);
		}
	}
}
