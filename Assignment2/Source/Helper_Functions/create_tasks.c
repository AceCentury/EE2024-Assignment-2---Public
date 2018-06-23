#include "create_tasks.h"
#include <stdio.h>

//FreeRTOS Includes
#include "FreeRTOS.h"
#include "task.h"

//Helper Includes
#include "oled_helpers.h"
#include "stationary_tasks.h"
#include "launch_tasks.h"
#include "return_tasks.h"
#include "my_uart.h"
#include "init_interfaces.h"

#define STACK_SIZE 200

// List of task handles
extern xTaskHandle enter_stationary_handle;
extern xTaskHandle temperature_stationary_handle;
extern xTaskHandle countdown_handle;
extern xTaskHandle temp_warning_stationary_handle;
extern xTaskHandle begin_countdown_handle;
extern xTaskHandle blink_red_handle;

extern xTaskHandle enter_launch_handle;
extern xTaskHandle temp_warning_launch_handle;
extern xTaskHandle accel_warning_handle;
extern xTaskHandle switch_to_return_handle;
extern xTaskHandle launch_mode_monitoring_handle;

extern xTaskHandle blink_blue_handle;
extern xTaskHandle blink_blue_and_red_handle;

extern xTaskHandle enter_return_handle;
extern xTaskHandle light_sensor_handle;
extern xTaskHandle switch_to_stationary_handle;
//extern xTaskHandle obstacle_near_handle;

extern xTaskHandle joystick_handle;
extern xTaskHandle send_NUSCloud_handle;

void joystick_task(void *);
void send_NUSCloud(void *);

// Bigger number = higher priority
void create_all_tasks() {

	xTaskCreate(enter_stationary, "Enter Stationary Mode", STACK_SIZE, NULL, 11,
			&enter_stationary_handle);

	xTaskCreate(temperature_stationary, "Temperature Update - Stationary",
			STACK_SIZE, NULL, 2, &temperature_stationary_handle);

	xTaskCreate(countdown, "Countdown", STACK_SIZE, NULL, 1, &countdown_handle);

	xTaskCreate(temp_warning_stationary, "Temp_Warning_Stationary", STACK_SIZE,
			NULL, 10, &temp_warning_stationary_handle);

	xTaskCreate(begin_countdown, "Begin Countdown", configMINIMAL_STACK_SIZE,
			NULL, 11, &begin_countdown_handle);

	xTaskCreate(blink_red, "blink_red", configMINIMAL_STACK_SIZE, NULL, 3,
			&blink_red_handle);

	xTaskCreate(enter_launch, "Enter Launch Mode", STACK_SIZE, NULL, 11,
			&enter_launch_handle);

	xTaskCreate(launch_mode_monitoring, "Launch mode monitoring", STACK_SIZE,
			NULL, 2, &launch_mode_monitoring_handle);

	xTaskCreate(temp_warning_launch, "Temp Warning - Launch", STACK_SIZE, NULL,
			10, &temp_warning_launch_handle);

	xTaskCreate(accel_warning, "Accel Warning", STACK_SIZE, NULL, 10,
			&accel_warning_handle);

	xTaskCreate(switch_to_return, "Switch to Return Task",
			configMINIMAL_STACK_SIZE, NULL, 11, &switch_to_return_handle);

	xTaskCreate(blink_blue, "Blink Blue", configMINIMAL_STACK_SIZE, NULL, 3,
			&blink_blue_handle);

	xTaskCreate(blink_blue_and_red, "Blink Blue and Red",
			configMINIMAL_STACK_SIZE, NULL, 3, &blink_blue_and_red_handle);

	xTaskCreate(enter_return, "Enter Return Mode", STACK_SIZE, NULL, 11,
			&enter_return_handle);

	xTaskCreate(light_sensor, "Light Sensor", STACK_SIZE, NULL, 2,
			&light_sensor_handle);

	xTaskCreate(switch_to_stationary, "Switch to stationary",
			configMINIMAL_STACK_SIZE, NULL, 11, &switch_to_stationary_handle);

	xTaskCreate(joystick_task, "Joystick", STACK_SIZE, NULL, 3,
			&joystick_handle);

	xTaskCreate(send_NUSCloud, "Send NUSCloud", STACK_SIZE, NULL, 4,
			&send_NUSCloud_handle);
}

extern uint8_t JOYSTICK_MODE;
extern int TEMP_WARNING_THRESHOLD;
extern float ACC_THRESHOLD;
extern uint32_t OBSTACLE_NEAR_THRESHOLD;
unsigned char output[15];
void joystick_task(void *ptr) {
	for (;;) {
		clearLineOled(6);
		if (JOYSTICK_MODE == 0) {
			sprintf(output, "Max Temp = %d", TEMP_WARNING_THRESHOLD);
		}

		else if (JOYSTICK_MODE == 1) {
			sprintf(output, "Max G = %.2f", ACC_THRESHOLD);
		}

		else if (JOYSTICK_MODE == 2) {
			sprintf(output, "Max Dist.= %d", OBSTACLE_NEAR_THRESHOLD);
		}
		printLineOled(output, 6);
		vTaskSuspend(NULL);
	}
}

extern unsigned char tempReading[15];
extern unsigned char xReading[20];
extern unsigned char yReading[20];
extern unsigned char light[30];
extern system_mode_t CURRENT_MODE;
volatile unsigned char tempOrLightData[30] = "";
volatile unsigned char xData[20] = "";
volatile unsigned char yData[20] = "";

void send_NUSCloud(void * ptr) {
	portTickType xLastWakeTime;
	const portTickType xFrequency = 10000;
	for (;;) {
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		if (CURRENT_MODE == LAUNCH) {
			sprintf(tempOrLightData, tempReading);
			UART_SendString(LPC_UART3, (uint8_t *) tempOrLightData);
			sprintf(xData, xReading);
			UART_SendString(LPC_UART3, (uint8_t *) xData);
			sprintf(yData, yReading);
			UART_SendString(LPC_UART3, (uint8_t *) yData);
		} else if (CURRENT_MODE == RETURN){
			sprintf(tempOrLightData, light);
			UART_SendString(LPC_UART3, (uint8_t *) tempOrLightData);
		}
	}
}
