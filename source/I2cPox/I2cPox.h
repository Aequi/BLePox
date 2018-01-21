#ifndef __I2C_POX_H__
#define __I2C_POX_H__

#include <stdint.h>
#include <stdbool.h>

typedef void (*I2cPoxIntCallback)(void);

void i2cPoxInit(I2cPoxIntCallback i2cIntCb);
uint8_t i2cPoxReadReg(uint8_t address);
void i2cPoxWriteReg(uint8_t address, uint8_t byte);

#endif
