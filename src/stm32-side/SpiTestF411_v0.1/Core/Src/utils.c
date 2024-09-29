#include <stdint.h>
/*
void utoc(uint8_t buffer, char* charBuffer){
  *charBuffer = '0' + buffer;
}
*/

void utoc(uint8_t buffer, char* charBuffer) {
  //static const char digits[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	static const char digits[10] = {"0123456789"};
  *charBuffer = digits[buffer];
}


/*
if (buffer < 10) {
	*charBuffer = digits[buffer];
} else {
	*charBuffer = '\0';
}
*/
