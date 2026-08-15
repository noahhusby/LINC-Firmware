//
// Created by Noah Husby on 6/11/26.
//

#include "tx_api.h"
#include "linc_gpib.h"

#include <stdint.h>

#include "main.h"
#include "stm32h523xx.h"
#include "stm32h5xx_hal_gpio.h"

#define LINC_GPIB_QUEUE_DEPTH 8

#define LINC_GPIB_STACK_SIZE 2048
#define LINC_GPIB_PRIORITY 10

static TX_THREAD linc_gpib_thread;
static VOID* linc_gpib_stack;

static TX_MUTEX linc_gpib_status_mutex;
static linc_gpib_status_t linc_gpib_status;

static TX_QUEUE linc_gpib_queue;
static ULONG linc_gpib_queue_storage[LINC_GPIB_QUEUE_DEPTH];

static VOID linc_gpib_thread_entry(ULONG thread_input);

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

static void linc_gpib_data_bus_receive(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    linc_gpib_enable_communication(false);

    GPIO_InitStruct.Pin = DIO1_Pin | DIO2_Pin | DIO3_Pin | DIO4_Pin | DIO5_Pin | DIO6_Pin | DIO7_Pin | DIO8_Pin;

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIB_DIR_GPIO_Port, GPIB_DIR_Pin, GPIO_PIN_RESET);

    linc_gpib_enable_communication(true);
}

static void linc_gpib_data_bus_transmit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    linc_gpib_enable_communication(false);

    HAL_GPIO_WritePin(GPIB_DIR_GPIO_Port, GPIB_DIR_Pin, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = DIO1_Pin | DIO2_Pin | DIO3_Pin | DIO4_Pin | DIO5_Pin | DIO6_Pin | DIO7_Pin | DIO8_Pin;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    linc_gpib_enable_communication(true);
}

static void linc_gpib_init_data_bus(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /*
     * Preload safe control states:
     * EN high  = disabled
     * DIR low  = GPIB -> MCU
     */
    HAL_GPIO_WritePin(GPIB_EN_GPIO_Port, GPIB_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIB_DIR_GPIO_Port, GPIB_DIR_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIB_EN_Pin | GPIB_DIR_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*
     * MCU data pins start as inputs.
     */
    GPIO_InitStruct.Pin = DIO1_Pin | DIO2_Pin | DIO3_Pin | DIO4_Pin | DIO5_Pin | DIO6_Pin | DIO7_Pin | DIO8_Pin;

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    linc_gpib_enable_communication(false);
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

static void linc_gpib_init(void)
{
    linc_gpib_init_data_bus();
    linc_gpib_init_management();
}

UINT linc_gpib_create(TX_BYTE_POOL* byte_pool)
{
    UINT status;

    if (byte_pool == NULL)
    {
        return TX_PTR_ERROR;
    }

    status = tx_mutex_create(&linc_gpib_status_mutex, "LINC GPIB Status", TX_INHERIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_queue_create(&linc_gpib_queue, "LINC GPIB Queue", TX_1_ULONG, linc_gpib_queue_storage,
                             sizeof(linc_gpib_queue_storage));

    if (status != TX_SUCCESS)
    {
        tx_mutex_delete(&linc_gpib_status_mutex);
        return status;
    }

    status = tx_byte_allocate(byte_pool, &linc_gpib_stack, LINC_GPIB_STACK_SIZE, TX_NO_WAIT);

    if (status != TX_SUCCESS)
    {
        tx_queue_delete(&linc_gpib_queue);
        tx_mutex_delete(&linc_gpib_status_mutex);
        return status;
    }

    status =
        tx_thread_create(&linc_gpib_thread, "LINC GPIB", linc_gpib_thread_entry, 0, linc_gpib_stack,
                         LINC_GPIB_STACK_SIZE, LINC_GPIB_PRIORITY, LINC_GPIB_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        tx_byte_release(linc_gpib_stack);
        tx_queue_delete(&linc_gpib_queue);
        tx_mutex_delete(&linc_gpib_status_mutex);
        return status;
    }

    return TX_SUCCESS;
}

static UINT linc_gpib_submit(linc_gpib_request_t* request)
{
    return tx_queue_send(&linc_gpib_queue, &request, TX_WAIT_FOREVER);
}

static linc_gpib_result_t linc_gpib_process_write(linc_gpib_request_t* request)
{
    /*
     * TODO:
     * Address instrument as listener.
     * Transmit request->tx_data.
     */

    return LINC_GPIB_OK;
}

static linc_gpib_result_t linc_gpib_process_read(linc_gpib_request_t* request)
{
    /*
     * TODO:
     * Address instrument as talker.
     * Receive into request->rx_data.
     */

    return LINC_GPIB_OK;
}

static linc_gpib_result_t linc_gpib_process_write_read(linc_gpib_request_t* request)
{
    /*
     * TODO:
     * Write command.
     * Turn bus around.
     * Read response.
     */

    return LINC_GPIB_OK;
}

static linc_gpib_result_t linc_gpib_process_clear(linc_gpib_request_t* request)
{
    (void)request;

    /*
     * TODO:
     * Implement GPIB device clear.
     */

    return LINC_GPIB_OK;
}

static void linc_gpib_process_request(linc_gpib_request_t* request)
{
    if (request == NULL)
    {
        return;
    }

    request->rx_length = 0;

    switch (request->type)
    {
    case LINC_GPIB_REQUEST_WRITE:
        request->result = linc_gpib_process_write(request);
        break;

    case LINC_GPIB_REQUEST_READ:
        request->result = linc_gpib_process_read(request);
        break;

    case LINC_GPIB_REQUEST_WRITE_READ:
        request->result = linc_gpib_process_write_read(request);
        break;

    case LINC_GPIB_REQUEST_CLEAR:
        request->result = linc_gpib_process_clear(request);
        break;

    default:
        request->result = LINC_GPIB_INVALID_ARGUMENT;
        break;
    }
}

static VOID linc_gpib_thread_entry(ULONG thread_input)
{
    (void)thread_input;

    linc_gpib_request_t* request = NULL;

    linc_gpib_init();

    while (1)
    {
        UINT status = tx_queue_receive(&linc_gpib_queue, &request, TX_TIMER_TICKS_PER_SECOND / 20);

        if (status == TX_SUCCESS)
        {
            linc_gpib_process_request(request);
            linc_gpib_update_status();
            tx_semaphore_put(&request->complete);
        }
        else if (status == TX_QUEUE_EMPTY)
        {
            linc_gpib_update_status();
        }
    }
}

linc_gpib_result_t linc_gpib_write(uint8_t address, const uint8_t* data, size_t length, bool send_eoi)
{
    if (data == NULL || length == 0 || address > 30)
    {
        return LINC_GPIB_INVALID_ARGUMENT;
    }

    linc_gpib_request_t request = {
        .type = LINC_GPIB_REQUEST_WRITE,
        .address = address,
        .tx_data = data,
        .tx_length = length,
        .rx_data = NULL,
        .rx_capacity = 0,
        .rx_length = 0,
        .send_eoi = send_eoi,
        .timeout_ticks = TX_TIMER_TICKS_PER_SECOND,
        .result = LINC_GPIB_ERROR,
    };

    if (tx_semaphore_create(&request.complete, "GPIB Request", 0) != TX_SUCCESS)
    {
        return LINC_GPIB_ERROR;
    }

    if (linc_gpib_submit(&request) != TX_SUCCESS)
    {
        tx_semaphore_delete(&request.complete);
        return LINC_GPIB_ERROR;
    }

    tx_semaphore_get(&request.complete, TX_WAIT_FOREVER);
    tx_semaphore_delete(&request.complete);

    return request.result;
}

linc_gpib_result_t linc_gpib_read(uint8_t address, uint8_t* data, size_t capacity, size_t* length)
{
    if (data == NULL || length == NULL || capacity == 0 || address > 30)
    {
        return LINC_GPIB_INVALID_ARGUMENT;
    }

    linc_gpib_request_t request = {
        .type = LINC_GPIB_REQUEST_READ,
        .address = address,
        .tx_data = NULL,
        .tx_length = 0,
        .rx_data = data,
        .rx_capacity = capacity,
        .rx_length = 0,
        .send_eoi = false,
        .timeout_ticks = TX_TIMER_TICKS_PER_SECOND,
        .result = LINC_GPIB_ERROR,
    };

    if (tx_semaphore_create(&request.complete, "GPIB Request", 0) != TX_SUCCESS)
    {
        return LINC_GPIB_ERROR;
    }

    if (linc_gpib_submit(&request) != TX_SUCCESS)
    {
        tx_semaphore_delete(&request.complete);
        return LINC_GPIB_ERROR;
    }

    tx_semaphore_get(&request.complete, TX_WAIT_FOREVER);

    *length = request.rx_length;

    tx_semaphore_delete(&request.complete);

    return request.result;
}

linc_gpib_result_t linc_gpib_query(uint8_t address, const uint8_t* tx_data, size_t tx_length, uint8_t* rx_data,
                                   size_t rx_capacity, size_t* rx_length)
{
    if (tx_data == NULL || tx_length == 0 || rx_data == NULL || rx_capacity == 0 || rx_length == NULL || address > 30)
    {
        return LINC_GPIB_INVALID_ARGUMENT;
    }

    linc_gpib_request_t request = {
        .type = LINC_GPIB_REQUEST_WRITE_READ,
        .address = address,
        .tx_data = tx_data,
        .tx_length = tx_length,
        .rx_data = rx_data,
        .rx_capacity = rx_capacity,
        .rx_length = 0,
        .send_eoi = true,
        .timeout_ticks = TX_TIMER_TICKS_PER_SECOND,
        .result = LINC_GPIB_ERROR,
    };

    if (tx_semaphore_create(&request.complete, "GPIB Request", 0) != TX_SUCCESS)
    {
        return LINC_GPIB_ERROR;
    }

    if (linc_gpib_submit(&request) != TX_SUCCESS)
    {
        tx_semaphore_delete(&request.complete);
        return LINC_GPIB_ERROR;
    }

    tx_semaphore_get(&request.complete, TX_WAIT_FOREVER);

    *rx_length = request.rx_length;

    tx_semaphore_delete(&request.complete);

    return request.result;
}