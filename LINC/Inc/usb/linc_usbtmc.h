#ifndef LINC_USBTMC_H
#define LINC_USBTMC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tx_api.h"
#include "ux_api.h"

/* Forward declaration to avoid coupling this header to the USBX class header. */
typedef struct UX_DEVICE_USBTMC_STRUCT UX_DEVICE_USBTMC;

#define LINC_USBTMC_THREAD_STACK_SIZE 2048U
#define LINC_USBTMC_THREAD_PRIORITY 10U

/*
 * USBX endpoint buffers are currently configured for 512 bytes.
 * This first implementation therefore handles one complete USBTMC
 * bulk message per USB transfer.
 */
#define LINC_USBTMC_BUFFER_SIZE 512U
#define LINC_USBTMC_HEADER_SIZE 12U

/* USBTMC bulk message IDs. */
#define LINC_USBTMC_MSGID_DEV_DEP_MSG_OUT 1U
#define LINC_USBTMC_MSGID_REQUEST_DEV_DEP_MSG_IN 2U
#define LINC_USBTMC_MSGID_DEV_DEP_MSG_IN 2U

/* USBTMC transfer attributes. */
#define LINC_USBTMC_TRANSFER_ATTRIBUTE_EOM 0x01U
#define LINC_USBTMC_TRANSFER_ATTRIBUTE_TERM_CHAR 0x02U

typedef enum
{
    LINC_USBTMC_RESULT_OK = 0,
    LINC_USBTMC_RESULT_INVALID_ARGUMENT,
    LINC_USBTMC_RESULT_INVALID_MESSAGE,
    LINC_USBTMC_RESULT_BUFFER_TOO_SMALL,
    LINC_USBTMC_RESULT_NOT_SUPPORTED,
    LINC_USBTMC_RESULT_APPLICATION_ERROR
} linc_usbtmc_result_t;

/*
 * Application callbacks.
 *
 * write:
 *   Called when the host sends DEV_DEP_MSG_OUT.
 *
 * read:
 *   Called when the host sends REQUEST_DEV_DEP_MSG_IN.
 *
 * These callbacks are where the later LINC/GPIB command layer attaches.
 */
typedef linc_usbtmc_result_t (*linc_usbtmc_write_callback_t)(const uint8_t* data, size_t length, bool end_of_message,
                                                             void* context);

typedef linc_usbtmc_result_t (*linc_usbtmc_read_callback_t)(uint8_t* data, size_t max_length, size_t* actual_length,
                                                            bool* end_of_message, void* context);

typedef struct
{
    linc_usbtmc_write_callback_t write;
    linc_usbtmc_read_callback_t read;
    void* context;
} linc_usbtmc_callbacks_t;

typedef struct
{
    bool connected;
    UX_DEVICE_USBTMC* device;

    TX_THREAD thread;
    VOID* thread_stack;

    UCHAR rx_buffer[LINC_USBTMC_BUFFER_SIZE];
    UCHAR tx_buffer[LINC_USBTMC_BUFFER_SIZE];

    linc_usbtmc_callbacks_t callbacks;
} linc_usbtmc_t;

/* Initialize local USBTMC application state. */
void linc_usbtmc_init(void);

/* Allocate resources and create the USBTMC worker thread. */
UINT linc_usbtmc_create(TX_BYTE_POOL* byte_pool);

/* USB connection state. */
bool linc_usbtmc_connected(void);

/* Called by ux_device_usbtmc on USBX class activation/deactivation. */
void linc_usbtmc_activate(UX_DEVICE_USBTMC* device);
void linc_usbtmc_deactivate(void);

/* Attach the higher-level LINC/GPIB handlers. */
void linc_usbtmc_set_callbacks(const linc_usbtmc_callbacks_t* callbacks);

/* USBTMC worker thread. */
VOID linc_usbtmc_thread_entry(ULONG thread_input);

#endif /* LINC_USBTMC_H */
