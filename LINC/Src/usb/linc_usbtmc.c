#include "linc_usbtmc.h"

#include <string.h>

#include "ux_device_usbtmc.h"

static linc_usbtmc_t usbtmc;

/* -------------------------------------------------------------------------- */
/* USBTMC helpers                                                             */
/* -------------------------------------------------------------------------- */

static uint32_t linc_usbtmc_read_u32_le(const uint8_t* data)
{
    return ((uint32_t)data[0]) | ((uint32_t)data[1] << 8U) | ((uint32_t)data[2] << 16U) | ((uint32_t)data[3] << 24U);
}

static void linc_usbtmc_write_u32_le(uint8_t* data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8U) & 0xFFU);
    data[2] = (uint8_t)((value >> 16U) & 0xFFU);
    data[3] = (uint8_t)((value >> 24U) & 0xFFU);
}

static bool linc_usbtmc_tag_is_valid(const uint8_t* message)
{
    const uint8_t b_tag = message[1];
    const uint8_t b_tag_inverse = message[2];

    if (b_tag == 0U)
    {
        return false;
    }

    return ((uint8_t)(b_tag ^ b_tag_inverse) == 0xFFU);
}

static size_t linc_usbtmc_padding_length(size_t payload_length) { return (4U - (payload_length & 0x03U)) & 0x03U; }

/* -------------------------------------------------------------------------- */
/* USBTMC bulk message handlers                                               */
/* -------------------------------------------------------------------------- */

static linc_usbtmc_result_t linc_usbtmc_handle_dev_dep_msg_out(const uint8_t* message, size_t message_length)
{
    const uint32_t transfer_size = linc_usbtmc_read_u32_le(&message[4]);

    const size_t padding_length = linc_usbtmc_padding_length((size_t)transfer_size);

    const size_t required_length = LINC_USBTMC_HEADER_SIZE + (size_t)transfer_size + padding_length;

    if (message_length < required_length)
    {
        return LINC_USBTMC_RESULT_INVALID_MESSAGE;
    }

    /*
     * DEV_DEP_MSG_OUT currently defines only EOM in bmTransferAttributes.
     */
    if ((message[8] & ~LINC_USBTMC_TRANSFER_ATTRIBUTE_EOM) != 0U)
    {
        return LINC_USBTMC_RESULT_INVALID_MESSAGE;
    }

    if (usbtmc.callbacks.write == NULL)
    {
        return LINC_USBTMC_RESULT_NOT_SUPPORTED;
    }

    const bool end_of_message = ((message[8] & LINC_USBTMC_TRANSFER_ATTRIBUTE_EOM) != 0U);

    return usbtmc.callbacks.write(&message[LINC_USBTMC_HEADER_SIZE], (size_t)transfer_size, end_of_message,
                                  usbtmc.callbacks.context);
}

static linc_usbtmc_result_t linc_usbtmc_handle_request_dev_dep_msg_in(const uint8_t* message, uint8_t* response_buffer,
                                                                      size_t response_capacity, size_t* response_length)
{
    const uint8_t b_tag = message[1];

    const uint32_t host_requested_length = linc_usbtmc_read_u32_le(&message[4]);

    /*
     * TermChar handling is intentionally not supported yet.
     */
    if ((message[8] & LINC_USBTMC_TRANSFER_ATTRIBUTE_TERM_CHAR) != 0U)
    {
        return LINC_USBTMC_RESULT_NOT_SUPPORTED;
    }

    if ((message[8] & ~LINC_USBTMC_TRANSFER_ATTRIBUTE_TERM_CHAR) != 0U)
    {
        return LINC_USBTMC_RESULT_INVALID_MESSAGE;
    }

    if (usbtmc.callbacks.read == NULL)
    {
        return LINC_USBTMC_RESULT_NOT_SUPPORTED;
    }

    if (response_capacity < LINC_USBTMC_HEADER_SIZE)
    {
        return LINC_USBTMC_RESULT_BUFFER_TOO_SMALL;
    }

    /*
     * Reserve the first 12 bytes for the USBTMC header. The application
     * writes its response payload directly after the header.
     */
    size_t payload_capacity = response_capacity - LINC_USBTMC_HEADER_SIZE;

    if (payload_capacity > (size_t)host_requested_length)
    {
        payload_capacity = (size_t)host_requested_length;
    }

    size_t payload_length = 0U;
    bool end_of_message = false;

    linc_usbtmc_result_t result = usbtmc.callbacks.read(&response_buffer[LINC_USBTMC_HEADER_SIZE], payload_capacity,
                                                        &payload_length, &end_of_message, usbtmc.callbacks.context);

    if (result != LINC_USBTMC_RESULT_OK)
    {
        return result;
    }

    if ((payload_length > payload_capacity) || (payload_length > UINT32_MAX))
    {
        return LINC_USBTMC_RESULT_APPLICATION_ERROR;
    }

    const size_t padding_length = linc_usbtmc_padding_length(payload_length);

    const size_t total_length = LINC_USBTMC_HEADER_SIZE + payload_length + padding_length;

    if (total_length > response_capacity)
    {
        return LINC_USBTMC_RESULT_BUFFER_TOO_SMALL;
    }

    /*
     * DEV_DEP_MSG_IN header.
     */
    response_buffer[0] = LINC_USBTMC_MSGID_DEV_DEP_MSG_IN;
    response_buffer[1] = b_tag;
    response_buffer[2] = (uint8_t)~b_tag;
    response_buffer[3] = 0U;

    linc_usbtmc_write_u32_le(&response_buffer[4], (uint32_t)payload_length);

    response_buffer[8] = end_of_message ? LINC_USBTMC_TRANSFER_ATTRIBUTE_EOM : 0U;

    response_buffer[9] = 0U;
    response_buffer[10] = 0U;
    response_buffer[11] = 0U;

    if (padding_length != 0U)
    {
        memset(&response_buffer[LINC_USBTMC_HEADER_SIZE + payload_length], 0, padding_length);
    }

    *response_length = total_length;

    return LINC_USBTMC_RESULT_OK;
}

static linc_usbtmc_result_t linc_usbtmc_process_bulk_out(const uint8_t* message, size_t message_length,
                                                         uint8_t* response_buffer, size_t response_capacity,
                                                         size_t* response_length)
{
    if ((message == NULL) || (response_buffer == NULL) || (response_length == NULL))
    {
        return LINC_USBTMC_RESULT_INVALID_ARGUMENT;
    }

    *response_length = 0U;

    if (message_length < LINC_USBTMC_HEADER_SIZE)
    {
        return LINC_USBTMC_RESULT_INVALID_MESSAGE;
    }

    if (!linc_usbtmc_tag_is_valid(message))
    {
        return LINC_USBTMC_RESULT_INVALID_MESSAGE;
    }

    /*
     * Byte 3 is reserved and must be zero.
     */
    if (message[3] != 0U)
    {
        return LINC_USBTMC_RESULT_INVALID_MESSAGE;
    }

    switch (message[0])
    {
    case LINC_USBTMC_MSGID_DEV_DEP_MSG_OUT:
        return linc_usbtmc_handle_dev_dep_msg_out(message, message_length);

    case LINC_USBTMC_MSGID_REQUEST_DEV_DEP_MSG_IN:
        return linc_usbtmc_handle_request_dev_dep_msg_in(message, response_buffer, response_capacity, response_length);

    default:
        return LINC_USBTMC_RESULT_NOT_SUPPORTED;
    }
}

/* -------------------------------------------------------------------------- */
/* Public lifecycle                                                          */
/* -------------------------------------------------------------------------- */

void linc_usbtmc_init(void)
{
    memset(&usbtmc, 0, sizeof(usbtmc));

    usbtmc.connected = false;
    usbtmc.device = UX_NULL;
}

UINT linc_usbtmc_create(TX_BYTE_POOL* byte_pool)
{
    UINT status = tx_byte_allocate(byte_pool, &usbtmc.thread_stack, LINC_USBTMC_THREAD_STACK_SIZE, TX_NO_WAIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_thread_create(&usbtmc.thread, "USBTMC", linc_usbtmc_thread_entry, 0, usbtmc.thread_stack,
                              LINC_USBTMC_THREAD_STACK_SIZE, LINC_USBTMC_THREAD_PRIORITY, LINC_USBTMC_THREAD_PRIORITY,
                              TX_NO_TIME_SLICE, TX_AUTO_START);

    return status;
}

bool linc_usbtmc_connected(void) { return usbtmc.connected; }

void linc_usbtmc_activate(UX_DEVICE_USBTMC* device)
{
    usbtmc.device = device;
    usbtmc.connected = true;
}

void linc_usbtmc_deactivate(void)
{
    usbtmc.connected = false;
    usbtmc.device = UX_NULL;
}

void linc_usbtmc_set_callbacks(const linc_usbtmc_callbacks_t* callbacks)
{
    if (callbacks == NULL)
    {
        memset(&usbtmc.callbacks, 0, sizeof(usbtmc.callbacks));

        return;
    }

    usbtmc.callbacks = *callbacks;
}

/* -------------------------------------------------------------------------- */
/* Worker thread                                                              */
/* -------------------------------------------------------------------------- */

VOID linc_usbtmc_thread_entry(ULONG thread_input)
{
    UX_PARAMETER_NOT_USED(thread_input);

    while (1)
    {
        if (!usbtmc.connected || usbtmc.device == UX_NULL)
        {
            tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10);

            continue;
        }

        ULONG actual_length = 0U;

        UINT status = ux_device_usbtmc_read(usbtmc.device, usbtmc.rx_buffer, sizeof(usbtmc.rx_buffer), &actual_length);

        if ((status != UX_SUCCESS) || (actual_length == 0U))
        {
            continue;
        }

        size_t response_length = 0U;

        linc_usbtmc_result_t result = linc_usbtmc_process_bulk_out(
            usbtmc.rx_buffer, (size_t)actual_length, usbtmc.tx_buffer, sizeof(usbtmc.tx_buffer), &response_length);

        if ((result != LINC_USBTMC_RESULT_OK) || (response_length == 0U))
        {
            continue;
        }

        ULONG written_length = 0U;

        status = ux_device_usbtmc_write(usbtmc.device, usbtmc.tx_buffer, (ULONG)response_length, &written_length);

        if (status != UX_SUCCESS)
        {
            continue;
        }
    }
}
