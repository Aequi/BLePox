#include "nrf_delay.h"

#include "ble_stack.h"
#include "gpio_hal.h"
#include "adc_hal.h"
#include "I2cPox.h"

#define BLE_HID_REPORT_SEND_RETRY_COUNT     200
static uint8_t poxData[19];
static volatile bool isAdcDataReady = false, isSendActive = false, isOverrun = false;

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

static void poxCb(void)
{
    isPoxInt = true;
}

static void NRF_SwitchToBleMode(bool eraseBonds)
{
    NRF_BleSetupHidReportReadyCb(onBleHidReportReceived);
    NRF_BleStart(eraseBonds);
}

static uint32_t wrPtr = 0, rdPtr = 0, ovr = 0;

int main(void)
{
    gpioHalInit();
    //adcHalInit(onAdcDataReady);

    nrf_delay_ms(1000);
    //gpioPwrOn(true);
    //while (gpioHalReadButton());
    scheduler_init();
    NRF_SwitchToBleMode(true);
    i2cPoxInit(poxCb);
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

        if (isPoxInt || i2cPoxGetIntStatus()) {

            static uint8_t samples[64];
            static uint32_t poxIntCntr = 0;
            static uint32_t sampleCntr = 0;
            bool isPoxDataReady = false;
            static uint8_t temp[2] = {0, 0};
            uint32_t delta = 0;
            static uint8_t poxIntStatus = 0;
            static bool blink = true;
            poxIntStatus = i2cPoxReadIntStatus();
            rdPtr = i2cPoxReadReadPtr();
            wrPtr = i2cPoxReadWritePtr();
            ovr = i2cPoxReadOvrPtr();
            if (rdPtr != wrPtr) {
                if (rdPtr > wrPtr) {
                    delta = 16 - rdPtr + wrPtr;
                } else {
                    delta = wrPtr - rdPtr;
                }

                i2cPoxReadData(&samples[sampleCntr * 4], delta);
                sampleCntr += delta;
                if (sampleCntr == 4) {
                    uint32_t bCntr = 0;
                    for (bCntr = 0; bCntr < 16; bCntr++) {
                        poxData[bCntr] = samples[bCntr];
                    }
                    sampleCntr = 0;
                    if (isPoxDataReady) {
                        blink = !blink;
                        gpioLedOnRed(blink);
                    }
                    isPoxDataReady = true;
                } else if (sampleCntr > 4) {
                    blink = !blink;
                    gpioLedOnRed(blink);
                    sampleCntr = 0;
                }
            }

            if (poxIntCntr++ == 300) {
                i2cPoxTriggerTemp();
            }

            if (poxIntCntr == 600) {
                i2cPoxReadTemp(temp);
                poxIntCntr = 0;
            }

            if (isPoxDataReady) {
                poxData[16] = temp[0];
                poxData[17] = temp[1];
                if (isSendActive) {
                    if (NRF_BleSendHidReport(poxData, BLE_HID_CUSTOM_REPORT_SIZE)) {
                        isPoxDataReady = false;
                    }
                    poxData[18]++;
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

