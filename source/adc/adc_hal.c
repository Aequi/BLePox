#include "adc_hal.h"

#include "nrf_drv_adc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "app_util_platform.h"
#include "nrf51_bitfields.h"
#include "nrf_drv_common.h"
#include "app_timer.h"

#define ADC_PIN         1
#define ADC_BUFF_SIZE   8
#define ADC_SAMPLE_RATE_DIVIDER				4000; // timer freq 500 KHz


static nrf_adc_value_t adcDataBuff[2][ADC_BUFF_SIZE];
static uint32_t adcInactiveBufIdx = 0;
static nrf_drv_adc_channel_t m_channel_config = NRF_DRV_ADC_DEFAULT_CHANNEL(NRF_ADC_CONFIG_INPUT_2);
static AdcDataReadyCallBack adcDataReadyCallBack;

static nrf_ppi_channel_t       m_ppi_channel;
static const nrf_drv_timer_t   m_timer = NRF_DRV_TIMER_INSTANCE(2);


static void adc_event_handler(nrf_drv_adc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_ADC_EVT_DONE) {
        adcInactiveBufIdx ^= 1;
        nrf_drv_adc_buffer_convert(adcDataBuff[adcInactiveBufIdx ^ 1], ADC_BUFF_SIZE);
            if (adcDataReadyCallBack)
        adcDataReadyCallBack();
    }
}


void timer_handler(nrf_timer_event_t event_type, void* p_context)
{
}

void adc_sampling_event_enable(void)
{
    ret_code_t err_code = nrf_drv_ppi_channel_enable(m_ppi_channel);
    APP_ERROR_CHECK(err_code);
}

void adc_sampling_event_init(void)
{
    ret_code_t err_code;
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_timer_init(&m_timer, NULL, timer_handler);
    APP_ERROR_CHECK(err_code);

    /* setup m_timer for compare event */
    uint32_t ticks = ADC_SAMPLE_RATE_DIVIDER;
    nrf_drv_timer_extended_compare(&m_timer, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    nrf_drv_timer_enable(&m_timer);

    uint32_t timer_compare_event_addr = nrf_drv_timer_compare_event_address_get(&m_timer, NRF_TIMER_CC_CHANNEL0);
    uint32_t adc_sample_event_addr = nrf_drv_adc_start_task_get();

    /* setup ppi channel so that timer compare event is triggering sample task in SAADC */
    err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_assign(m_ppi_channel, timer_compare_event_addr, adc_sample_event_addr);  //NRF_ADC->TASKS_START);
    APP_ERROR_CHECK(err_code);
}

void adcHalInit(AdcDataReadyCallBack adcDataReadyCb)
{
    adcDataReadyCallBack = adcDataReadyCb;

    adc_sampling_event_init();

    nrf_drv_adc_config_t config = NRF_DRV_ADC_DEFAULT_CONFIG;
    nrf_drv_adc_init(&config, adc_event_handler);
    m_channel_config.config.config.input = NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD;
    nrf_drv_adc_channel_enable(&m_channel_config);

    nrf_drv_adc_buffer_convert(adcDataBuff[0], ADC_BUFF_SIZE);
    adcInactiveBufIdx = 1;

    adc_sampling_event_enable();
}

const uint8_t * adcGetData(void)
{
    return (uint8_t *) adcDataBuff[adcInactiveBufIdx];
}

