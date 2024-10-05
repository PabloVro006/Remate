#ifndef INC_HALL_H_
#define INC_HALL_H_

#include <stdbool.h>
#include "config.h"

bool hallCheck(GPIO_TypeDef* port, const uint16_t pin);

#endif /* INC_HALL_H_ */
