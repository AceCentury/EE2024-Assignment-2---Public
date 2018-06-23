#include "oled_helpers.h"

/* Library includes. */
#include "LPC17xx.h"

#include "oled.h"

uint8_t *BLANK_LINE = "               ";

//Print different readings on different lines. LINES 0 - 6
void printLineOled(unsigned char* readings, int line) {
	oled_putString(0, line * 9, readings, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
}

void clearLineOled(int line) { // LINES 0 - 6
	oled_putString(0, line * 9, BLANK_LINE, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

}
