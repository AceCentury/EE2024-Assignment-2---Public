#include "return_tasks.h"
#include <stdio.h>

//FreeRTOS Includes
#include "FreeRTOS.h"
#include "task.h"

//LPC Board Includes
#include "lpc17xx.h"

#include "pca9532.h"
#include "oled.h"
#include "light.h"

//Helper Includes
#include "oled_helpers.h"
#include "my_rgb.h"
#include "led7seg.h"
#include "init_interfaces.h"
#include "my_uart.h"

extern uint32_t OBSTACLE_NEAR_THRESHOLD;
extern int CURRENT_MODE;
extern int MODE_TOGGLE_PRESSES;
extern int TEMP_WARNING;
extern int TEMP_WARNING_THRESHOLD;

extern int ACC_WARNING;

// Declaring variables
uint8_t *RETURN_TEXT = "RETURN";
//static char* msg = NULL;
unsigned char light[30] = "";
unsigned char obstacle_near_message[15] = "";

volatile warning_state OBST_WARNING;

extern uint8_t *TEMP_WARNING_TEXT;
extern uint8_t *TEMP_SAFE_TEXT;
extern int32_t CURR_TEMP;
extern char msg[15];

extern xTaskHandle temp_warning_launch_handle;
extern xTaskHandle accel_warning_handle;
extern xTaskHandle switch_to_return_handle;
extern xTaskHandle launch_mode_monitoring_handle;

extern xTaskHandle blink_blue_handle;
extern xTaskHandle blink_red_handle;
extern xTaskHandle blink_blue_and_red_handle;

extern xTaskHandle light_sensor_handle;
extern xTaskHandle enter_stationary_handle;
extern xTaskHandle obstacle_near_handle;

extern xTaskHandle joystick_handle;

uint16_t ledArray = 0xffff;

uint16_t encodeLEDArray(uint32_t value);

void enter_return(void *ptr) {
	for (;;) {
		vTaskSuspend(NULL);
		sprintf(msg, "Entering RETURN mode \r\n");
		UART_SendString(LPC_UART3, (uint8_t *) msg);
		vTaskSuspend(switch_to_return_handle);
		vTaskSuspend(blink_blue_handle);
		vTaskSuspend(blink_red_handle);
		vTaskSuspend(blink_blue_and_red_handle);
		vTaskSuspend(launch_mode_monitoring_handle);
		//Clear Previous Warnings
		RGB_RED_OFF();
		RGB_BLUE_OFF();
		TEMP_WARNING = OFF;
		ACC_WARNING = OFF;
		OBST_WARNING = OFF;
		vTaskResume(joystick_handle);
		vTaskResume(light_sensor_handle);
		CURRENT_MODE = RETURN; //Set RETURN mode
		oled_clearScreen(OLED_COLOR_BLACK);
		printLineOled(RETURN_TEXT, 0);
		MODE_TOGGLE_PRESSES = 0;
		led7seg_setChar('0', FALSE);
	}
}

void light_sensor(void *ptr) {
	vTaskSuspend(NULL);

	portTickType xLastWakeTime;

	const portTickType xFrequency = 70;
	for (;;) {
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		uint32_t lightReading = light_read();
		if (lightReading >= OBSTACLE_NEAR_THRESHOLD && OBST_WARNING == OFF) {
			OBST_WARNING = ON;
			sprintf(obstacle_near_message, "Obstacle near!\r\n");
			UART_SendString(LPC_UART3, (uint8_t *) obstacle_near_message);
			printLineOled(obstacle_near_message, 2);
		}
		else if (OBST_WARNING == ON && lightReading < OBSTACLE_NEAR_THRESHOLD) {
			sprintf(msg, "Obstacle avoided!\r\n");
			UART_SendString(LPC_UART3, (uint8_t *) msg);
			OBST_WARNING = OFF;
			clearLineOled(2);
		}
		ledArray = encodeLEDArray(lightReading);
		pca9532_setLeds(ledArray, 0xffff);
		sprintf(light, "Obstacle Distance is %d \r\n", lightReading);
	}
}

void switch_to_stationary(void *ptr) {
	int last_press_count;
	int curr_press_count;
	int count;
	portTickType xLastWakeTime;
	const portTickType xFrequency = 1000;
	vTaskSuspend(NULL);
	for (;;) {
		last_press_count = MODE_TOGGLE_PRESSES;
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		curr_press_count = MODE_TOGGLE_PRESSES;
		count = curr_press_count - last_press_count;
		if (count == 0) { //Set to detect no extra press because the first press would unblock this task.
			pca9532_setLeds(0, 0xffff);// OFF all LEDs
			vTaskResume(enter_stationary_handle);
		}
		MODE_TOGGLE_PRESSES = 0; //Prevent overflow
		vTaskSuspend(NULL);
	}
}

uint16_t encodeLEDArray(uint32_t value) { //Every 200 lux light up 1 LED. Assume range is 64000 lux and 16 bit.
	uint16_t multiplier = 200; //LED light up interval
	uint32_t adjustedValue = value;
	uint16_t result = 0x0001;
	uint16_t activateBit = 0x0002;
	while (adjustedValue > multiplier) {
		result |= activateBit; //Activate next LED
		activateBit = activateBit << 1; //Bitshift for next LED
		adjustedValue -= multiplier;
	}
	return result;
}
