#include "nrf_delay.h"

#include "ble_stack.h"
#include "gpio_hal.h"
#include "adc_hal.h"
#include "I2cPox.h"

#define BLE_HID_REPORT_SEND_RETRY_COUNT     200

static volatile bool isAdcDataReady = false, isSendActive = false, isOverrun = false;

static void onBleHidReportReceived(const uint8_t report[], uint32_t length)
{
    if (report) {
        if (report[1]) {
            isSendActive = true;
        } else {
            isSendActive = false;
        }
    } else {
        isSendActive = false;
    }
}
/*
static void onAdcDataReady(void)
{
    if (isAdcDataReady && isSendActive) {
        isOverrun = true;
//        gpioLedOnRed(false);
    }
    isAdcDataReady = true;
}
*/

static bool isPoxInt = false;
static uint8_t poxIntStatus = 0;

static void poxCb(void)
{
    poxIntStatus = i2cPoxReadIntStatus();
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
    //adcHalInit(onAdcDataReady);

    nrf_delay_ms(1000);
    //gpioPwrOn(true);
    //while (gpioHalReadButton());
    i2cPoxInit(poxCb);
    scheduler_init();
    NRF_SwitchToBleMode(true);
    uint32_t cntr = 0;
    for (;;) {
        /*
        if (isAdcDataReady && isSendActive) {
            for (uint32_t i = 0; i < BLE_HID_REPORT_SEND_RETRY_COUNT; i++) {
                if (NRF_BleSendHidReport(adcGetData(), BLE_HID_CUSTOM_REPORT_SIZE)) {
                    isAdcDataReady = false;
                    break;
                }
            }
        }
        */

        if (isPoxInt) {
            static uint8_t poxData[19];
            static uint8_t samples[16];
            static uint32_t poxIntCntr = 0;
            static uint32_t sampleCntr = 0;
            bool isPoxDataReady = false;
            uint32_t wrPtr = 0, rdPtr = 0;
            uint8_t temp[2] = {0, 0};
            uint32_t delta = 0;

            rdPtr = i2cPoxReadReadPtr();
            wrPtr = i2cPoxReadWritePtr();
            if (rdPtr != wrPtr) {
                if (rdPtr > wrPtr) {
                    delta = 16 - rdPtr + wrPtr;
                } else {
                    delta = wrPtr - rdPtr;
                }

                i2cPoxReadData(&samples[sampleCntr], delta);
                sampleCntr += delta;
                if (sampleCntr == 16) {
                    uint32_t cntr = 0;
                    for (cntr = 0; cntr < 16; cntr++) {
                        poxData[cntr] = samples[cntr];
                    }
                    sampleCntr = 0;
                    isPoxDataReady = true;
                }
            }

            if (poxIntCntr++ == 300) {
                i2cPoxTriggerTemp();
            }

            if (poxIntCntr == 600) {
                i2cPoxReadTemp(temp);
            }

            if (isPoxDataReady) {
                poxData[16] = temp[0];
                poxData[17] = temp[1];

                if (NRF_BleSendHidReport(poxData, BLE_HID_CUSTOM_REPORT_SIZE)) {
                    isPoxDataReady = false;
                }
            }

            isPoxInt = false;
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

