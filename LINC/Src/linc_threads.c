//
// Created by Noah Husby on 6/11/26.
//

#include "linc_threads.h"

#include "linc_status_led.h"

#define LINC_STATUS_LED_STACK_SIZE 512
#define LINC_STATUS_LED_PRIORITY 20

static TX_THREAD linc_status_led_thread;
static ULONG linc_status_led_stack[LINC_STATUS_LED_STACK_SIZE / sizeof(ULONG)];

UINT linc_threads_create(void)
{
    const UINT status = tx_thread_create(&linc_status_led_thread, "LINC Status LED", linc_status_led_thread_entry, 0,
                                         linc_status_led_stack, sizeof(linc_status_led_stack), LINC_STATUS_LED_PRIORITY,
                                         LINC_STATUS_LED_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
    return status;
}