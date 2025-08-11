#ifndef PUSH_SW_H
#define PUSH_SW_H

#include "gpio.h"

uint16_t readPushSwitch(GPIO_TypeDef* port, uint16_t pin);

#endif /* PUSH_SW_H */
