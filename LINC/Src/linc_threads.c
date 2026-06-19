//
// Created by Noah Husby on 6/11/26.
//

#include "linc_threads.h"

#include <stdint.h>

#include "linc_gpib.h"
#include "linc_status_led.h"

#define LINC_STATUS_LED_STACK_SIZE 512
#define LINC_STATUS_LED_PRIORITY 20

#define LINC_GPIB_STACK_SIZE 512
#define LINC_GPIB_PRIORITY 15

static TX_THREAD linc_status_led_thread;
static uint8_t linc_status_led_stack[LINC_STATUS_LED_STACK_SIZE];

static TX_THREAD linc_gpib_thread;
static uint8_t linc_gpib_stack[LINC_GPIB_STACK_SIZE];

UINT linc_threads_create(void)
{
    UINT status = tx_thread_create(&linc_status_led_thread, "LINC Status LED", linc_status_led_thread_entry,
                                         0, linc_status_led_stack, LINC_STATUS_LED_STACK_SIZE,
                                         LINC_STATUS_LED_PRIORITY, LINC_STATUS_LED_PRIORITY, 1, TX_AUTO_START);

    status = tx_thread_create(&linc_gpib_thread, "LINC GPIB", linc_gpib_thread_entry, 0, linc_gpib_stack, LINC_GPIB_STACK_SIZE, LINC_GPIB_PRIORITY, LINC_GPIB_PRIORITY, 1, TX_AUTO_START);
    return status;
}