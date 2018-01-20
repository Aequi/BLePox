#ifndef __ADC_HAL_H__
#define __ADC_HAL_H__

#include <stdint.h>
#include <stdbool.h>


typedef void (*AdcDataReadyCallBack)(void);

void adcHalInit(AdcDataReadyCallBack adcDataReadyCb);
const uint8_t * adcGetData(void);

#endif
