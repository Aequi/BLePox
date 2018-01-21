#include "I2cPox.h"
#include "nrf_drv_twi.h"
#include "nrf_drv_gpiote.h"
#include "app_util_platform.h"
#include "nrf51_bitfields.h"
#include "nrf_drv_common.h"

#include <stdlib.h>
#include <string.h>

#define TWI_SCL_PIN         2
#define TWI_SDA_PIN         3
#define INT_PIN             4
#define I2C_INSTANCE        0
#define POX_ADDRESS         0xAE
#define I2C_POX_BUFF_SIZE   32

static const nrf_drv_twi_t m_twi_master = NRF_DRV_TWI_INSTANCE(I2C_INSTANCE);
static I2cPoxIntCallback i2cPoxIntCallback = NULL;

void intEventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (action == NRF_GPIOTE_POLARITY_HITOLO) {
        if (i2cPoxIntCallback != NULL) {
            i2cPoxIntCallback();
        }
    }
}

static void i2cRead(uint8_t addr, uint8_t data[], size_t size)
{
    static uint8_t addr8b;
    addr8b = addr;
    nrf_drv_twi_tx(&m_twi_master, POX_ADDRESS, &addr8b, 1, true);
    nrf_drv_twi_rx(&m_twi_master, POX_ADDRESS, data, size);
}

static void i2cWrite(uint8_t addr, uint8_t const data[], size_t size)
{
    static uint8_t buffer[1 + I2C_POX_BUFF_SIZE];
    buffer[0] = addr;
    memcpy(buffer + 1, data, size);
    nrf_drv_twi_tx(&m_twi_master, POX_ADDRESS, buffer, size + 1, false);
}

uint8_t i2cPoxReadReg(uint8_t address)
{
    static uint8_t regVal;
    i2cRead(address, &regVal, 1);
    return regVal;
}

void i2cPoxWriteReg(uint8_t address, uint8_t byte)
{
    static uint8_t regVal;
    regVal = byte;
    i2cWrite(address, &regVal, 1);
}

void i2cPoxInit(I2cPoxIntCallback i2cPoxIntCb)
{
    i2cPoxIntCallback = i2cPoxIntCb;
    const nrf_drv_twi_config_t twiConfig = {
       .scl                = TWI_SCL_PIN,
       .sda                = TWI_SDA_PIN,
       .frequency          = NRF_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
    };

    const nrf_drv_gpiote_in_config_t gpioteConfig = {
        .is_watcher = false,
        .hi_accuracy = true,
        .pull = NRF_GPIO_PIN_PULLUP,
        .sense = NRF_GPIOTE_POLARITY_HITOLO,
    };

    nrf_drv_gpiote_init();
    nrf_drv_gpiote_in_init(INT_PIN, &gpioteConfig, intEventHandler);

    nrf_drv_gpiote_in_event_enable(INT_PIN, true);
    nrf_drv_twi_init(&m_twi_master, &twiConfig, NULL, NULL);

    nrf_drv_twi_enable(&m_twi_master);

    uint8_t id = i2cPoxReadReg(0xFF);
    if (id == 0x11) {
        volatile bool ok = true;
    }

    i2cPoxWriteReg(0x06, 0x03); // SPO2 mode
    i2cPoxWriteReg(0x07, 0x47); // 16 bit, 100sps
    i2cPoxWriteReg(0x09, 0x33); // SPO2 mode
    i2cPoxWriteReg(0x01, 0x0F); // Enable interrupts
}

