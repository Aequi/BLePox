#ifndef __I2C_POX_H__
#define __I2C_POX_H__

#include <stdint.h>
#include <stdbool.h>

typedef void (*I2cPoxIntCallback)(void);

void i2cPoxInit(I2cPoxIntCallback i2cIntCb);

uint8_t i2cPoxReadIntStatus(void);
uint8_t i2cPoxReadReadPtr(void);
uint8_t i2cPoxReadOvrPtr(void);
uint8_t i2cPoxReadWritePtr(void);
void i2cPoxWriteReadPtr(uint8_t val);
void i2cPoxWriteOvrPtr(uint8_t val);
void i2cPoxWriteWritePtr(uint8_t val);

void i2cPoxReadData(uint8_t samples[], uint32_t count);
void i2cPoxTriggerTemp(void);
void i2cPoxReadTemp(uint8_t temp[]);

bool i2cPoxGetIntStatus(void);

#endif
