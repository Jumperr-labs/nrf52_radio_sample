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
* @defgroup nrf_dev_button_radio_tx_example_main main.c
* @{
* @ingroup nrf_dev_button_radio_tx_example
*
* @brief Radio Transceiver Example Application main file.
*
* This file contains the source code for a sample application using the NRF_RADIO peripheral.
*
*/


#include "nrf.h"
#include "nrf_drv_saadc.h"
#include "nrf_delay.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "radio_config.h"
#include "nrf_gpio.h"
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "nordic_common.h"
#include "nrf_error.h"
#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "nrf_log_ctrl.h"

#define APP_TIMER_PRESCALER      0                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE  2                           /**< Size of timer operation queues. */

static uint32_t                   packet;                    /**< Packet to transmit. */


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
            packet = temperature;
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


/**@brief Function for sending packet.
 */
void send_packet()
{
    // send the packet:
    NRF_RADIO->EVENTS_READY = 0U;
    NRF_RADIO->TASKS_TXEN   = 1;

    while (NRF_RADIO->EVENTS_READY == 0U)
    {
        // wait
    }
    NRF_RADIO->EVENTS_END  = 0U;
    NRF_RADIO->TASKS_START = 1U;

    while (NRF_RADIO->EVENTS_END == 0U)
    {
        // wait
    }

    uint32_t err_code = bsp_indication_set(BSP_INDICATE_SENT_OK);
    // NRF_LOG_INFO("The packet was sent\r\n");
    APP_ERROR_CHECK(err_code);

    NRF_RADIO->EVENTS_DISABLED = 0U;
    // Disable radio
    NRF_RADIO->TASKS_DISABLE = 1U;

    while (NRF_RADIO->EVENTS_DISABLED == 0U)
    {
        // wait
    }
}


/**@brief Function for handling bsp events.
 */
void bsp_evt_handler(bsp_event_t evt)
{
    uint32_t prep_packet = 0;
    switch (evt)
    {
        case BSP_EVENT_KEY_0:
            /* Fall through. */
        case BSP_EVENT_KEY_1:
            /* Fall through. */
        case BSP_EVENT_KEY_2:
            /* Fall through. */
        case BSP_EVENT_KEY_3:
            /* Fall through. */
        case BSP_EVENT_KEY_4:
            /* Fall through. */
        case BSP_EVENT_KEY_5:
            /* Fall through. */
        case BSP_EVENT_KEY_6:
            /* Fall through. */
        case BSP_EVENT_KEY_7:
            /* Get actual button state. */
            for (int i = 0; i < BUTTONS_NUMBER; i++)
            {
                prep_packet |= (bsp_board_button_state_get(i) ? (1 << i) : 0);
            }
            break;
        default:
            /* No implementation needed. */
            break;
    }
    packet = prep_packet;
}


/**@brief Function for initialization oscillators.
 */
void clock_initialization()
{
    /* Start 16 MHz crystal oscillator */
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    /* Wait for the external oscillator to start up */
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
    {
        // Do nothing.
    }

    /* Start low frequency crystal oscillator for app_timer(used by bsp)*/
    NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;

    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    {
        // Do nothing.
    }
}


/**
 * @brief Function for application main entry.
 * @return 0. int return type required by ANSI/ISO standard.
 */
int main(void)
{
    uint32_t err_code = NRF_SUCCESS;

    adc_config();

    clock_initialization();
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS,
                        APP_TIMER_TICKS(100, APP_TIMER_PRESCALER),
                        bsp_evt_handler);
    APP_ERROR_CHECK(err_code);

    // Set radio configuration parameters
    radio_configure();

    // Set payload pointer
    NRF_RADIO->PACKETPTR = (uint32_t)&packet;

    err_code = bsp_indication_set(BSP_INDICATE_USER_STATE_OFF);
    // NRF_LOG_INFO("Press Any Button\r\n");
    APP_ERROR_CHECK(err_code);

    while (true)
    {
        nrf_delay_ms(100000);
        
        APP_ERROR_CHECK(nrf_drv_saadc_buffer_convert(adc_buffer,1));
        nrf_drv_saadc_sample();

        send_packet();
        NRF_LOG_INFO("Sent temperature: %d\r\n", (int)packet);
        NRF_LOG_FLUSH();
    }
}


/**
 *@}
 **/
