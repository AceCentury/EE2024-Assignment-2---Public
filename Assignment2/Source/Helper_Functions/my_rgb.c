#include "my_rgb.h"

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"

void myRGB_init(void) {
	PINSEL_CFG_Type PinCfg;
	//Setting P2.0 RGB RED
	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 1;
	PinCfg.Pinmode = 1;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(2, 0, 1);

	PinCfg.Funcnum = 0;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 1;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 26;
	PINSEL_ConfigPin(&PinCfg);
	GPIO_SetDir(0, (1 << 26), 1);

}
void RGB_RED_ON(void) {
	GPIO_SetValue(2, 1 << 0);
}

void RGB_RED_OFF(void) {
	GPIO_ClearValue(2, (1 << 0));
}
void RGB_BLUE_ON(void) {
	GPIO_SetValue(0, (1 << 26));
}
void RGB_BLUE_OFF(void) {
	GPIO_ClearValue(0, (1 << 26));
}
