#include "fonts.h"
#include "ssd1306.h"

void startDisplay(){
	SSD1306_Init();

	SSD1306_GotoXY (0,25);
	SSD1306_Puts ("REMATE", &Font_11x18, 1);
	SSD1306_UpdateScreen();
	HAL_Delay (1000);

	SSD1306_ScrollRight(0,4);
	HAL_Delay(1000);
	SSD1306_ScrollLeft(0,4);
	HAL_Delay(1000);
	SSD1306_Stopscroll();
	SSD1306_Clear();
}

void printBufferData(uint8_t buffer){
	char charBuffer;

	SSD1306_Clear();
	SSD1306_GotoXY (35,0);
	SSD1306_Puts ("DATA:", &Font_11x18, 1);
	SSD1306_GotoXY (8,28);
	SSD1306_Puts ("[", &Font_16x26, 1);
	SSD1306_GotoXY (109,28);
	SSD1306_Puts ("]", &Font_16x26, 1);

	itoa((int)buffer, &charBuffer, 10);
	SSD1306_GotoXY (60, 30);
	SSD1306_Puts(&charBuffer, &Font_11x18, 1);
	SSD1306_UpdateScreen();
	HAL_Delay(10);
}
