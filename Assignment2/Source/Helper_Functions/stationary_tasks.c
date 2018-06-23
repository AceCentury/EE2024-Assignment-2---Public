#include "stationary_tasks.h"
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
#include "led7seg.h"
#include "init_interfaces.h"
#include "my_uart.h"

extern int CURRENT_MODE;
extern int MODE_TOGGLE_PRESSES;
extern int TEMP_WARNING;
extern int TEMP_WARNING_THRESHOLD;

// Values for inverted 7 segment
uint8_t segmentA = 0x28;
uint8_t segmentB = 0x23;
uint8_t segmentC = 0xA6;
uint8_t segmentD = 0x61;
uint8_t segmentE = 0xA2;
uint8_t segmentF = 0xAA;
uint8_t segment9 = 0x30;
uint8_t segment8 = 0x20;
uint8_t segment7 = 0x7C;
uint8_t segment6 = 0x22;
uint8_t segment5 = 0x32;
uint8_t segment4 = 0x39;
uint8_t segment3 = 0x70;
uint8_t segment2 = 0xE0;
uint8_t segment1 = 0x7D;

//Declaring variables
uint8_t *STATIONARY_TEXT = "STATIONARY";
extern uint8_t *TEMP_WARNING_TEXT;
extern uint8_t *TEMP_SAFE_TEXT;

extern int32_t CURR_TEMP;

int countdown_timer;
unsigned char msg[20] = "";
unsigned char tempReading[15] = "";

extern xTaskHandle enter_stationary_handle;
extern xTaskHandle temperature_stationary_handle;
extern xTaskHandle countdown_handle;
extern xTaskHandle temp_warning_stationary_handle;
extern xTaskHandle blink_red_handle;
extern xTaskHandle begin_countdown_handle;

extern xTaskHandle enter_launch_handle;

extern xTaskHandle light_sensor_handle;

extern xTaskHandle joystick_handle;

void enter_stationary(void *ptr) { //Task to enter stationary mode
	for (;;) {
		//vTaskSuspend(send_NUSCloud_handle);
		sprintf(msg, "Entering STATIONARY mode \r\n");
		UART_SendString(LPC_UART3, (uint8_t *) msg);
		CURRENT_MODE = STATIONARY;
		TEMP_WARNING = OFF;
		oled_clearScreen(OLED_COLOR_BLACK);
		printLineOled(STATIONARY_TEXT, 0);
		led7seg_setChar(segmentF, TRUE);
		countdown_timer = 15;
		printLineOled(TEMP_SAFE_TEXT, 2);
		vTaskResume(temperature_stationary_handle);
		vTaskSuspend(NULL );
		vTaskSuspend(light_sensor_handle);
		vTaskResume(joystick_handle);
	}
}

void temperature_stationary(void *ptr) { //FUEL TANK MONITORING
	float temperature = 0;
	int8_t SW4;
	temperature = (CURR_TEMP / 10.0);
	sprintf(tempReading, "Temp = %.2f", temperature);
	printLineOled(tempReading, 1);

	portTickType xLastWakeTime;
	const portTickType xFrequency = 70;
	for (;;) {

		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);

		//Read SW4 for CLEAR_WARNING
		SW4 = (GPIO_ReadValue(1) >> 31) & 1;
		if (TEMP_WARNING == 1 && (SW4 == 0)) {
			vTaskSuspend(blink_red_handle);
			RGB_RED_OFF();
			clearLineOled(2);
			printLineOled(TEMP_SAFE_TEXT, 2);
			TEMP_WARNING = OFF;
		}
		temperature = (CURR_TEMP / 10.0); //Getting latest temp and converting it
		if ((temperature > TEMP_WARNING_THRESHOLD) && TEMP_WARNING == OFF) { //Trigger Warning Sequence
			TEMP_WARNING = ON;
			vTaskResume(temp_warning_stationary_handle);
		}
		sprintf(tempReading, "Temp = %.2f", temperature);
		printLineOled(tempReading, 1);

	}
}

void countdown(void *ptr) {
	portTickType xLastWakeTime;
	const portTickType xFrequency = 1000;
	vTaskSuspend(NULL );
	for (;;) {
		countdown_timer--;
		switch (countdown_timer) {
		case 14:
			led7seg_setChar(segmentE, TRUE);
			break;
		case 13:
			led7seg_setChar(segmentD, TRUE);
			break;
		case 12:
			led7seg_setChar(segmentC, TRUE);
			break;
		case 11:
			led7seg_setChar(segmentB, TRUE);
			break;
		case 10:
			led7seg_setChar(segmentA, TRUE);
			break;
		case 9:
			led7seg_setChar(segment9, TRUE);
			break;
		case 8:
			led7seg_setChar(segment8, TRUE);
			break;
		case 7:
			led7seg_setChar(segment7, TRUE);
			break;
		case 6:
			led7seg_setChar(segment6, TRUE);
			break;
		case 5:
			led7seg_setChar(segment5, TRUE);
			break;
		case 4:
			led7seg_setChar(segment4, TRUE);
			break;
		case 3:
			led7seg_setChar(segment3, TRUE);
			break;
		case 2:
			led7seg_setChar(segment2, TRUE);
			break;
		case 1:
			led7seg_setChar(segment1, TRUE);
			break;
		case 0:
			led7seg_setChar('0', FALSE);
			vTaskResume(enter_launch_handle);
			break;
		}
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

void temp_warning_stationary(void *ptr) {
	for (;;) {
		vTaskSuspend(NULL );
		printLineOled(TEMP_WARNING_TEXT, 2);
		led7seg_setChar(segmentF, TRUE);
		vTaskSuspend(countdown_handle);
		countdown_timer = 15;
		vTaskResume(blink_red_handle);
	}
}

void begin_countdown(void *ptr) {
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
		if (count == 0 && TEMP_WARNING == OFF) { //Set to detect no extra press because the first press would unblock this task.
			vTaskResume(countdown_handle);
		}
		MODE_TOGGLE_PRESSES = 0; //Prevent overflow
		vTaskSuspend(NULL );
	}
}

void blink_red(void *ptr) {
	vTaskSuspend(NULL );
	portTickType xLastWakeTime;
	const portTickType xFrequency = 333;
	for (;;) {
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		RGB_RED_ON();
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		RGB_RED_OFF();
	}
}

