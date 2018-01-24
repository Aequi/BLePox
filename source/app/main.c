#include "nrf_delay.h"

#include "ble_stack.h"
#include "gpio_hal.h"
#include "adc_hal.h"
#include "I2cPox.h"

#define BLE_HID_REPORT_SEND_RETRY_COUNT     200

static volatile bool isAdcDataReady = false, isSendActive = false, isOverrun = false;
static uint32_t wrPtr = 0, rdPtr = 0, ovr = 0;
static uint8_t poxData[19];
static uint8_t ringBuffData[256];
static uint32_t wrPtrB = 0;
static uint32_t rdPtrB = 0;

static void onBleHidReportReceived(const uint8_t report[], uint32_t length)
{
    if (report) {
        if (report[1]) {
            isSendActive = true;
        } else {
            isSendActive = false;
            poxData[18] = 0;
        }
    } else {
        isSendActive = false;
    }
}


static volatile bool isPoxInt = false;

static void poxCb(void)
{
    isPoxInt = true;
}

static void NRF_SwitchToBleMode(bool eraseBonds)
{
    NRF_BleSetupHidReportReadyCb(onBleHidReportReceived);
    NRF_BleStart(eraseBonds);
}

int main(void)
{
    gpioHalInit();
    nrf_delay_ms(1000);
    scheduler_init();
    NRF_SwitchToBleMode(false);
    i2cPoxInit(poxCb);

    static uint32_t poxIntCntr = 0;
    static uint8_t temp[2] = {0, 0};
    uint32_t delta = 0;
    uint32_t delta2 = 0;
    uint32_t cntr = 0;

    for (;;) {
        if (isPoxInt || i2cPoxGetIntStatus()) {

            i2cPoxReadIntStatus();
            rdPtr = i2cPoxReadReadPtr();
            wrPtr = i2cPoxReadWritePtr();
            ovr = i2cPoxReadOvrPtr();
            if (rdPtr != wrPtr) {
                if (rdPtr > wrPtr) {
                    delta = 16 - rdPtr + wrPtr;
                } else {
                    delta = wrPtr - rdPtr;
                }

                i2cPoxReadData(&ringBuffData[wrPtrB], delta);
                wrPtrB += delta * 4;
                if (wrPtrB >= 256) {
                    wrPtrB = 0;
                }
            }

            if (poxIntCntr++ == 10000) {
                i2cPoxTriggerTemp();
            }

            if (poxIntCntr == 20000) {
                i2cPoxReadTemp(temp);
                poxIntCntr = 0;
            }

            isPoxInt = false;
        }
        delta2 = 0;
        if (rdPtrB != wrPtrB) {
            if (rdPtrB > wrPtrB) {
                delta2 = 256 - rdPtrB + wrPtrB;
            } else {
                delta2 = wrPtrB - rdPtrB;
            }
        }

        if (delta2 >= 16) {
            uint32_t bCntr = 0;
            for (bCntr = 0; bCntr < 16; bCntr++) {
                poxData[bCntr] = ringBuffData[rdPtrB++];
                if (rdPtrB >= 256)
                    rdPtrB = 0;
            }

            poxData[16] = temp[0];
            poxData[17] = temp[1];

            if (isSendActive) {
                if (!NRF_BleSendHidReport(poxData, BLE_HID_CUSTOM_REPORT_SIZE)) {
                    if (!NRF_BleSendHidReport(poxData, BLE_HID_CUSTOM_REPORT_SIZE)) {
                        if (!NRF_BleSendHidReport(poxData, BLE_HID_CUSTOM_REPORT_SIZE)) {
                            if (!NRF_BleSendHidReport(poxData, BLE_HID_CUSTOM_REPORT_SIZE)) {

                            }
                        }
                    }
                }
                poxData[18]++;
            }

        }

        if (cntr == 10000) {
            gpioLedOn(true);
        }
        if (cntr++ == 15000) {
            gpioLedOn(false);
            cntr = 0 - 30000;
        }

        NRF_BleProcess();
    }
}

