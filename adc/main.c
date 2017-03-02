/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup blinky_example_main main.c
 * @{
 * @ingroup blinky_example
 * @brief Blinky Example Application main file.
 *
 * This file contains the source code for a sample application to blink LEDs.
 *
 */

#include "nrf.h"
#include "nrf_drv_saadc.h"
#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"


#define ADC_BUFFER_SIZE 10                                /**< Size of buffer for ADC samples.  */
static nrf_saadc_value_t       adc_buffer[ADC_BUFFER_SIZE]; /**< ADC buffer. */

/**
 * @brief ADC interrupt handler.
 */
void saadc_event_handler(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        uint32_t i;
        for (i = 0; i < p_event->data.done.size; i++)
        {
            // NRF_LOG_INFO("Current sample value: %d\r\n", p_event->data.done.p_buffer[i]);
            uint16_t sample = p_event->data.done.p_buffer[i];
            int32_t temperature = (sample * 165.405405)/1024 - 49;
            NRF_LOG_INFO("Current sample value: %d\r\n", temperature);
        }
    }
}

/**
 * @brief ADC initialization.
 */
static void adc_config(void)
{
    ret_code_t ret_code;    

    ret_code = nrf_drv_saadc_init(NULL, saadc_event_handler);
    APP_ERROR_CHECK(ret_code);
  
    nrf_saadc_channel_config_t config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN0);
    ret_code = nrf_drv_saadc_channel_init(0, &config);
        
        
}


/**
 * @brief Function for application main entry.
 */
int main(void)
{
    /* Configure board. */
    bsp_board_leds_init();
    adc_config();
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

    // NRF_LOG_INFO("ADC example\r\n");
    /* Toggle LEDs. */
    while (true)
    {
    // NRF_LOG_INFO("ADC example\r\n");
        
        for (int i = 0; i < LEDS_NUMBER; i++)
        {
			APP_ERROR_CHECK(nrf_drv_saadc_buffer_convert(adc_buffer,1));
			nrf_drv_saadc_sample();
            bsp_board_led_invert(i);
            nrf_delay_ms(500);
        }
    }
}

/**
 *@}
 **/
