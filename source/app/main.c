#include "nrf_delay.h"
#include "BleStack.h"
/*
#include "gpio_hal.h"
#include "adc_hal.h"
#include "I2cPox.h"


static uint32_t wrPtr = 0, rdPtr = 0, ovr = 0;

static uint8_t ringBuffData[256];
static uint32_t wrPtrB = 0;
static uint32_t rdPtrB = 0;

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
*/
uint8_t cntr = 0;
static volatile bool isAdcDataReady = false, isSendActive = false, isOverrun = false;
static uint8_t poxData[19];

static void onBleDataReceived(const uint8_t data[], uint32_t length)
{
    if (data) {
        if (data[1]) {
            isSendActive = true;
        } else {
            isSendActive = false;
            poxData[18] = 0;
        }
        cntr = data[0] - '0';
    } else {
        isSendActive = false;
    }
}


int main(void)
{
    bleStackStart(onBleDataReceived);

    uint8_t text[] = {'T', 'i', 'm', 'e', ':', ' ', ' ', ' '};
    for (;;) {
        nrf_delay_ms(1000);

        text[7] = cntr % 10 + '0';
        text[6] = cntr / 10 + '0';

        if (cntr++ >= 59) {
            cntr = 0;
        }

        bleStackWrite(text, sizeof(text));
    }
    /*
    gpioHalInit();
    nrf_delay_ms(1000);
    scheduler_init();
    NRF_SwitchToBleMode(true);
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
                while (delta--) {
                    uint8_t data[4];
                    i2cPoxReadData(data, 1);

                    //i2cPoxReadData(&ringBuffData[wrPtrB], delta);
                    ringBuffData[wrPtrB++] = data[0];
                    ringBuffData[wrPtrB++] = data[1];
                    ringBuffData[wrPtrB++] = data[2];
                    ringBuffData[wrPtrB++] = data[3];

                    if (wrPtrB >= 256) {
                        wrPtrB = 0;
                    }
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
    */
}


