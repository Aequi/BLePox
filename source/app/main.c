#include "nrf_delay.h"
#include "BleStack.h"
#include "I2cPox.h"

static volatile bool isPoxInt = false;

static void poxCb(void)
{
    isPoxInt = true;
}

static volatile bool isSendActive = false;
static uint8_t poxData[20];

static void onBleDataReceived(const uint8_t data[], uint32_t length)
{
    if (data) {
        if (data[0]) {
            isSendActive = true;
        } else {
            isSendActive = false;
            poxData[18] = 0;
        }
    } else {
        isSendActive = false;
    }
}

int main(void)
{
    static uint8_t ringBuffData[256];
    uint32_t wrPtrB = 0;
    uint32_t rdPtrB = 0;

    uint8_t temperature[2] = {0, 0};

    uint32_t poxIntCntr = 0;

    uint32_t wrPtr = 0, rdPtr = 0;

    uint32_t delta = 0;
    uint32_t delta2 = 0;

    nrf_delay_ms(1000);
    bleStackStart(onBleDataReceived);
    i2cPoxInit(poxCb);

    for (;;) {
        if (isPoxInt || i2cPoxGetIntStatus()) {
            i2cPoxReadIntStatus();
            rdPtr = i2cPoxReadReadPtr();
            wrPtr = i2cPoxReadWritePtr();
            i2cPoxReadOvrPtr();
            if (rdPtr != wrPtr) {
                if (rdPtr > wrPtr) {
                    delta = 16 - rdPtr + wrPtr;
                } else {
                    delta = wrPtr - rdPtr;
                }
                while (delta--) {
                    uint8_t data[4];
                    i2cPoxReadData(data, 1);
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

            if (poxIntCntr == 15000) {
                i2cPoxReadTemp(temperature);
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

            poxData[16] = temperature[0];
            poxData[17] = temperature[1];

            if (isSendActive) {
                if (bleStackWrite(poxData, sizeof(poxData)) != 0) {
                    if (bleStackWrite(poxData, sizeof(poxData)) != 0) {
                        if (!bleStackWrite(poxData, sizeof(poxData)) != 0) {
                            if (!bleStackWrite(poxData, sizeof(poxData)) != 0) {

                            }
                        }
                    }
                }
                poxData[18]++;
            }
        }
    }
}


