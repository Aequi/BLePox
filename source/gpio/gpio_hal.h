#ifndef __GPIO_HAL_H__
#define __HPIO_HAL_H__

#include <stdint.h>
#include <stdbool.h>


void gpioHalInit(void);
bool gpioHalReadButton(void);
void gpioPwrOn(bool isOn);
void gpioLedOn(bool isOn);

#endif
