//
// Created by Noah Husby on 6/11/26.
//

#include "linc_threads.h"

#include <stdint.h>

#include "linc_gpib.h"
#include "linc_status_led.h"
#include "../Inc/usb/linc_usb.h"

#define LINC_STATUS_LED_STACK_SIZE 512
#define LINC_STATUS_LED_PRIORITY 20

#define LINC_GPIB_STACK_SIZE 512
#define LINC_GPIB_PRIORITY 15

#define LINC_USB_STACK_SIZE 1024
#define LINC_USB_PRIORITY 16

static TX_THREAD linc_status_led_thread;
static uint8_t linc_status_led_stack[LINC_STATUS_LED_STACK_SIZE];

static TX_THREAD linc_gpib_thread;
static uint8_t linc_gpib_stack[LINC_GPIB_STACK_SIZE];

UINT linc_threads_create(TX_BYTE_POOL* byte_pool)
{
    UINT status = tx_thread_create(&linc_status_led_thread, "LINC Status LED", linc_status_led_thread_entry, 0,
                                   linc_status_led_stack, LINC_STATUS_LED_STACK_SIZE, LINC_STATUS_LED_PRIORITY,
                                   LINC_STATUS_LED_PRIORITY, 1, TX_AUTO_START);

    status = tx_thread_create(&linc_gpib_thread, "LINC GPIB", linc_gpib_thread_entry, 0, linc_gpib_stack,
                              LINC_GPIB_STACK_SIZE, LINC_GPIB_PRIORITY, LINC_GPIB_PRIORITY, 1, TX_AUTO_START);
    linc_usb_init();
    if (linc_usb_create(byte_pool) != TX_SUCCESS)
    {
        return TX_THREAD_ERROR;
    }
    return status;
}