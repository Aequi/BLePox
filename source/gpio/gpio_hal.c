#include "nrf_gpio.h"

#define GPIO_PWR_PIN        9
#define GPIO_LED_PIN        13//22
#define GPIO_BUT_PIN        10
#define GPIO_LED_PIN_R      21//22

void gpioHalInit(void)
{
     nrf_gpio_cfg_output(GPIO_PWR_PIN);
     nrf_gpio_cfg_output(GPIO_LED_PIN);
     nrf_gpio_cfg_input(GPIO_BUT_PIN, NRF_GPIO_PIN_PULLDOWN);
}

bool gpioHalReadButton(void)
{
    return (bool) nrf_gpio_pin_read(GPIO_BUT_PIN);
}

void gpioPwrOn(bool isOn)
{
    if (isOn)
        nrf_gpio_pin_set(GPIO_PWR_PIN);
    else
        nrf_gpio_pin_clear(GPIO_PWR_PIN);
}

void gpioLedOn(bool isOn)
{
    if (isOn)
        nrf_gpio_pin_set(GPIO_LED_PIN);
    else
        nrf_gpio_pin_clear(GPIO_LED_PIN);
}

void gpioLedOnRed(bool isOn)
{
         nrf_gpio_cfg_output(GPIO_LED_PIN_R);
    if (isOn)
        nrf_gpio_pin_set(GPIO_LED_PIN_R);
    else
        nrf_gpio_pin_clear(GPIO_LED_PIN_R);
}
