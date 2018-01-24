#include "I2cPox.h"
#include "nrf_drv_twi.h"
#include "nrf_drv_gpiote.h"
#include "app_util_platform.h"
#include "nrf51_bitfields.h"
#include "nrf_drv_common.h"

#include <stdlib.h>
#include <string.h>

#define TWI_SCL_PIN         15
#define TWI_SDA_PIN         16
#define INT_PIN             17
#define I2C_INSTANCE        0
#define POX_ADDRESS         (0xAE >> 1)
#define I2C_POX_BUFF_SIZE   256

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

    nrf_gpio_pin_dir_set(TWI_SCL_PIN, NRF_GPIO_PIN_DIR_INPUT);
    nrf_gpio_pin_dir_set(TWI_SDA_PIN, NRF_GPIO_PIN_DIR_INPUT);
    nrf_gpio_cfg_input(INT_PIN, NRF_GPIO_PIN_PULLUP);

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

    if (!nrf_drv_gpiote_is_init()) {
        nrf_drv_gpiote_init();
    }
    nrf_drv_gpiote_in_init(INT_PIN, &gpioteConfig, intEventHandler);
    nrf_drv_gpiote_in_event_enable(INT_PIN, true);

    nrf_drv_twi_init(&m_twi_master, &twiConfig, NULL, NULL);
    nrf_drv_twi_enable(&m_twi_master);

    i2cPoxWriteReg(0x06, 0x40); // Reset
    while (i2cPoxReadReg(0x00) & 0x01) {

    }

    i2cPoxWriteReg(0x06, 0x03); // SPO2 mode
    i2cPoxWriteReg(0x09, 0x77); // 24 mA current
    i2cPoxWriteReg(0x07, 0x47); // 16 bit, 100sps
    i2cPoxWriteReg(0x01, 0x10); // Enable interrupts

    i2cPoxWriteReg(0x02, 0x00);
    i2cPoxWriteReg(0x03, 0x00);
    i2cPoxWriteReg(0x04, 0x00);
    i2cPoxReadReg(0x00);
}

bool i2cPoxGetIntStatus(void)
{
    return ((bool) !nrf_gpio_pin_read(INT_PIN));
}

uint8_t i2cPoxReadIntStatus(void)
{
    return i2cPoxReadReg(0x00);
}

uint8_t i2cPoxReadReadPtr(void)
{
    return i2cPoxReadReg(0x04);
}

uint8_t i2cPoxReadOvrPtr(void)
{
    return i2cPoxReadReg(0x03);
}

uint8_t i2cPoxReadWritePtr(void)
{
    return i2cPoxReadReg(0x02);
}

void i2cPoxWriteReadPtr(uint8_t val)
{
    i2cPoxWriteReg(0x04, val);
}

void i2cPoxWriteOvrPtr(uint8_t val)
{
    i2cPoxWriteReg(0x03, val);
}

void i2cPoxWriteWritePtr(uint8_t val)
{
    i2cPoxWriteReg(0x02, val);
}

void i2cPoxReadData(uint8_t samples[], uint32_t count)
{
    return i2cRead(0x05, samples, count * 4);
}

void i2cPoxTriggerTemp(void)
{
    i2cPoxWriteReg(0x06, 0x0B);
}

void i2cPoxReadTemp(uint8_t temp[])
{
    return i2cRead(0x16, temp, 2);
}
