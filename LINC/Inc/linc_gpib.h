//
// Created by Noah Husby on 6/11/26.
//

#pragma once

#ifndef LINC_FIRMWARE_LINC_GPIB_H
#define LINC_FIRMWARE_LINC_GPIB_H
#include <stdbool.h>

#include "tx_port.h"

#endif // LINC_FIRMWARE_LINC_GPIB_H

typedef struct
{
    bool srq;
    bool ndac;
    bool nrfd;
    bool dav;
    bool eoi;
} linc_gpib_status_t;

VOID linc_gpib_thread_entry(ULONG thread_input);

linc_gpib_status_t linc_gpib_get_status(void);
