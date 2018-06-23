#include "init_interfaces.h"

#include <string.h>
#include <stdio.h>

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_uart.h"

#include "joystick.h"
#include "pca9532.h"
#include "oled.h"
#include "temp.h"
#include "led7seg.h"
#include "light.h"
#include "acc.h"

#include "my_rgb.h"

#include "my_uart.h"

// Define for Accelerometer setup
#define I2CDEV LPC_I2C2
#define ACC_I2C_ADDR    (0x1D)
#define ACC_ADDR_CTL1   0x18
#define ACC_ADDR_YOFFL  0x12
#define ACC_ADDR_XOFFL  0x10
//End define

void setup_acc(void);
void init_ssp(void);
void init_i2c(void);
void init_GPIO(void);
void joystick_interrupt_init(void);
void init_uart(void);
void pinsel_uart3(void);
void UART_ISR_Receive(void);

//Public method to setup interfaces
void setup_interfaces() {
	init_i2c();
	init_ssp();
	init_GPIO();

	pca9532_init();
	joystick_init();
	joystick_interrupt_init();

	setup_acc();

	oled_init();
	myRGB_init();
	led7seg_init();
	// Light Sensor
	light_enable();
	light_setRange(LIGHT_RANGE_4000);
	light_setWidth(LIGHT_WIDTH_12BITS);
	// Light Sensor End
	init_uart();
}

//Private functions

static int I2CWrite(uint8_t addr, uint8_t* buf, uint32_t len) {
	I2C_M_SETUP_Type txsetup;

	txsetup.sl_addr7bit = addr;
	txsetup.tx_data = buf;
	txsetup.tx_length = len;
	txsetup.rx_data = NULL;
	txsetup.rx_length = 0;
	txsetup.retransmissions_max = 3;

	if (I2C_MasterTransferData(I2CDEV, &txsetup, I2C_TRANSFER_POLLING)
			== SUCCESS) {
		return (0);
	} else {
		return (-1);
	}
}

void setup_acc(void) {
	acc_init();
	acc_setRange(ACC_RANGE_2G);
	acc_setMode(ACC_MODE_MEASURE);
	// Disable Z-axis
	uint8_t buf[2];
	buf[0] = ACC_ADDR_CTL1;
	buf[1] = 0x05;
	I2CWrite(ACC_I2C_ADDR, buf, 2);

	// X-offset
	buf[0] = ACC_ADDR_XOFFL;
	buf[1] = 0x09;
	I2CWrite(ACC_I2C_ADDR, buf, 2);

	// Y-Offset
	buf[0] = ACC_ADDR_YOFFL;
	buf[1] = 0x30;
	I2CWrite(ACC_I2C_ADDR, buf, 2);
}

void init_ssp(void) {
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);
}

void init_i2c(void) {
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

void init_GPIO(void) {
	//Setup SW4
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 1;
	PinCfg.Pinnum = 31;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(1, 1 << 31, 0);

	// Set up SW3 as EINT0
	PinCfg.Funcnum = 1;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 10;
	PINSEL_ConfigPin(&PinCfg);
}

void joystick_interrupt_init(void) {
	// Enable and clear GPIO Interrupt P2.4 Joystick Left
	LPC_GPIOINT ->IO2IntEnF |= 1 << 4;
	LPC_GPIOINT ->IO2IntClr = 1 << 4;

	// Enable and clear GPIO Interrupt P0.16 Joystick Right
	LPC_GPIOINT ->IO0IntEnF |= 1 << 16;
	LPC_GPIOINT ->IO0IntClr = 1 << 16;

	// Enable and clear GPIO Interrupt P2.3 Joystick Up
	LPC_GPIOINT ->IO2IntEnF |= 1 << 3;
	LPC_GPIOINT ->IO2IntClr = 1 << 3;

	// Enable and clear GPIO Interrupt P0.15 Joystick Down
	LPC_GPIOINT ->IO0IntEnF |= 1 << 15;
	LPC_GPIOINT ->IO0IntClr = 1 << 15;

	// Enable and clear GPIO Interrupt P0.17 Joystick Center
	LPC_GPIOINT ->IO0IntEnF |= 1 << 17;
	LPC_GPIOINT ->IO0IntClr = 1 << 17;

}

void pinsel_uart3(void) {
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 0;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);
}
void init_uart(void) {

	UART_CFG_Type uartCfg;
	UART_FIFO_CFG_Type uartFIFOCfg;
	uartCfg.Baud_rate = 115200;
	uartCfg.Databits = UART_DATABIT_8;
	uartCfg.Parity = UART_PARITY_NONE;
	uartCfg.Stopbits = UART_STOPBIT_1;
	pinsel_uart3();
	UART_Init(LPC_UART3, &uartCfg);
	UART_FIFOConfigStructInit(&uartFIFOCfg);
	uartFIFOCfg.FIFO_Level = UART_FIFO_TRGLEV0; // set to trigger interrupt at 1 character
	UART_FIFOConfig(LPC_UART3, &uartFIFOCfg);
	UART_SetupCbs(LPC_UART3, 0, (void *) UART_ISR_Receive);
	UART_IntConfig(LPC_UART3, UART_INTCFG_RBR, ENABLE);
	UART_TxCmd(LPC_UART3, ENABLE);
	NVIC_EnableIRQ(UART3_IRQn);
}

// UART Receive Callback Function - it will be called when a message is received
volatile uint8_t receivedCommand[4];
unsigned char mycommand[4] = "RPT\0";
uint8_t data = 0;
uint8_t len = 0;
extern system_mode_t CURRENT_MODE;
extern unsigned char tempReading[15];
extern unsigned char xReading[20];
extern unsigned char yReading[20];
extern unsigned char light[30];
extern volatile unsigned char tempOrLightData[30];
extern volatile unsigned char xData[20];
extern volatile unsigned char yData[20];

void UART_ISR_Receive(void) {
	/* Read the received data */
	UART_Receive(LPC_UART3, &data, 1, BLOCKING);
	if (data != '\r') {
		receivedCommand[len] = data;
		len++;
	}

	else if (data == '\r') {
		receivedCommand[3] = '\0';
		if (strcmp((char *) receivedCommand, (char *) mycommand) == 0) { //Check if data received is RPT
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
			receivedCommand[0] = '\0'; //Clearing the string
		}
		len = 0;
	}

	if (len == 3)
		len = 0; //Refresh Command Scanning
}

