#include "nrf_delay.h"

#include "ble_stack.h"
#include "gpio_hal.h"
#include "adc_hal.h"


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

static void onAdcDataReady(void)
{
    if (isAdcDataReady && isSendActive) {
        isOverrun = true;
        gpioLedOnRed(false);
    }
    isAdcDataReady = true;
}

static void NRF_SwitchToBleMode(bool eraseBonds)
{
    NRF_BleSetupHidReportReadyCb(onBleHidReportReceived);
    NRF_BleStart(eraseBonds);
}

int main(void)
{
    gpioHalInit();
    adcHalInit(onAdcDataReady);

    nrf_delay_ms(1000);
    //gpioPwrOn(true);
    //while (gpioHalReadButton());

    scheduler_init();
    NRF_SwitchToBleMode(true);

    uint32_t cntr = 0;
    for (;;) {

        if (isAdcDataReady && isSendActive) {
            for (uint32_t i = 0; i < BLE_HID_REPORT_SEND_RETRY_COUNT; i++)
                if (NRF_BleSendHidReport(adcGetData(), BLE_HID_CUSTOM_REPORT_SIZE)) {
                    isAdcDataReady = false;
                    break;
                }
        }

        if (cntr == 10000) {
            gpioLedOn(true);
        }

        if (cntr++ == 15000) {
            gpioLedOn(false);
            cntr = 0 - 30000;
        }
        //if (gpioHalReadButton())
        //    gpioPwrOn(false);

        NRF_BleProcess();
    }
}

