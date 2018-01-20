#ifndef BLE_STACK_H
#define BLE_STACK_H

#include <stdint.h>
#include <stdbool.h>

#define BLE_HID_CUSTOM_REPORT_SIZE      19

typedef void (*NRF_BleHidReportReadyCb)(const uint8_t report[], uint32_t length);

void NRF_BleProcess(void);
bool NRF_BleSendHidReport(const uint8_t report[], uint32_t length);
void NRF_BleSetupHidReportReadyCb(NRF_BleHidReportReadyCb callBack);

void NRF_BleStart(bool eraseBonds);
void NRF_BleStop(void);
bool NRF_BleSendMousePos(uint8_t dXlow, uint8_t dXhigh, uint8_t dYlow, uint8_t dYhigh);

void scheduler_init(void);

#endif
