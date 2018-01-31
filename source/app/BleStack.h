#ifndef __BLE_STACK_H__
#define __BLE_STACK_H__

#include <stdint.h>
#include <stdbool.h>

typedef void (*BleStackDataReadCallback)(const uint8_t data[], uint32_t length);

uint32_t bleStackStart(BleStackDataReadCallback bleStackDataReadCb);
bool bleStackWrite(const uint8_t data[], uint32_t length);

#endif
