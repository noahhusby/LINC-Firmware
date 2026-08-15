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

static void linc_gpib_enable_communication(bool enable)
{
    HAL_GPIO_WritePin(GPIB_EN_GPIO_Port, GPIB_EN_Pin, enable ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void linc_gpib_set_direction(enum CommunicationDirection direction)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (direction == CONTROLLER_TO_INSTRUMENT)
    {
        HAL_GPIO_WritePin(GPIB_DIR_GPIO_Port, GPIB_DIR_Pin, GPIO_PIN_RESET);
        GPIO_InitStruct.Pin = DIO1_Pin | DIO2_Pin | DIO3_Pin | DIO4_Pin | DIO5_Pin | DIO6_Pin | DIO7_Pin | DIO8_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }
    else
    {
        GPIO_InitStruct.Pin = DIO1_Pin | DIO2_Pin | DIO3_Pin | DIO4_Pin | DIO5_Pin | DIO6_Pin | DIO7_Pin | DIO8_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIB_DIR_GPIO_Port, GPIB_DIR_Pin, GPIO_PIN_SET);
    }
}

static void linc_gpib_init_data_bus(void)
{
    // Pre-define comms settings
    linc_gpib_enable_communication(false);
    linc_gpib_set_direction(CONTROLLER_TO_INSTRUMENT);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIB_EN_Pin | GPIB_DIR_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void linc_gpib_init_management(void)
{
    HAL_GPIO_WritePin(iNRFD_GPIO_Port, iNRFD_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(iNDAC_GPIO_Port, iNDAC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(iIFC_GPIO_Port, iIFC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(iREN_GPIO_Port, iREN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(iEOI_GPIO_Port, iEOI_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(iDAV_GPIO_Port, iDAV_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(iATN_GPIO_Port, iATN_Pin, GPIO_PIN_SET);

    // GPIOB: iNRFD, iNDAC, iIFC, iREN, iEOI, iDAV
    GPIO_InitTypeDef GPIOB_InitStruct = {0};
    GPIOB_InitStruct.Pin = iNDAC_Pin | iNRFD_Pin | iIFC_Pin | iREN_Pin | iEOI_Pin | iDAV_Pin;
    GPIOB_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIOB_InitStruct.Pull = GPIO_NOPULL;
    GPIOB_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIOB_InitStruct);

    // GPIOC: iATN
    GPIO_InitTypeDef GPIOC_InitStruct = {0};
    GPIOC_InitStruct.Pin = iATN_Pin;
    GPIOC_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIOC_InitStruct.Pull = GPIO_NOPULL;
    GPIOC_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIOC_InitStruct);
}

void linc_gpib_init(void)
{
    linc_gpib_init_data_bus();
    linc_gpib_init_management();
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
