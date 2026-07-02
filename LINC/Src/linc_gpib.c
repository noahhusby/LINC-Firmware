//
// Created by Noah Husby on 6/11/26.
//

#include "tx_api.h"
#include "linc_gpib.h"

#include <stdint.h>

#include "main.h"
#include "stm32h523xx.h"
#include "stm32h5xx_hal_gpio.h"
static TX_MUTEX linc_gpib_status_mutex;
static linc_gpib_status_t linc_gpib_status;

static bool read_pin(GPIO_TypeDef* port, uint16_t pin) { return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET; }

static void linc_gpib_update_status(void)
{
    linc_gpib_status_t status = {
        .srq = read_pin(SRQ_SENSE_GPIO_Port, SRQ_SENSE_Pin),
        .ndac = read_pin(NDAC_SENSE_GPIO_Port, NDAC_SENSE_Pin),
        .nrfd = read_pin(NRFD_SENSE_GPIO_Port, NRFD_SENSE_Pin),
        .dav = read_pin(DAV_SENSE_GPIO_Port, DAV_SENSE_Pin),
        .eoi = read_pin(EOI_SENSE_GPIO_Port, EOI_SENSE_Pin),
    };
    tx_mutex_get(&linc_gpib_status_mutex, TX_WAIT_FOREVER);
    linc_gpib_status = status;
    tx_mutex_put(&linc_gpib_status_mutex);
}

linc_gpib_status_t linc_gpib_get_status(void)
{

    tx_mutex_get(&linc_gpib_status_mutex, TX_WAIT_FOREVER);
    linc_gpib_status_t status = linc_gpib_status;
    tx_mutex_put(&linc_gpib_status_mutex);
    return status;
}

void linc_gpib_init_direction(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIB_EN_Pin | GPIB_DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Default states

}

VOID linc_gpib_thread_entry(ULONG thread_input)
{
    tx_mutex_create(&linc_gpib_status_mutex, "LINC GPIB Status", TX_NO_INHERIT);
    while (1)
    {
        linc_gpib_update_status();
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 20);
    }
}
