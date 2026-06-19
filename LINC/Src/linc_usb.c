//
// Created by Noah Husby on 6/18/26.
//

#include "linc_usb.h"

#include "tx_api.h"

VOID linc_usb_thread_entry(ULONG thread_input)
{
    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}