//
// Created by Noah Husby on 6/11/26.
//

#pragma once

#ifndef LINC_FIRMWARE_LINC_GPIB_H
#define LINC_FIRMWARE_LINC_GPIB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tx_api.h"

typedef struct
{
    bool srq;
    bool ndac;
    bool nrfd;
    bool dav;
    bool eoi;
} linc_gpib_status_t;

typedef enum
{
    LINC_GPIB_OK = 0,
    LINC_GPIB_ERROR,
    LINC_GPIB_TIMEOUT,
    LINC_GPIB_INVALID_ARGUMENT,
    LINC_GPIB_BUFFER_TOO_SMALL,
} linc_gpib_result_t;

typedef enum
{
    LINC_GPIB_REQUEST_WRITE,
    LINC_GPIB_REQUEST_READ,
    LINC_GPIB_REQUEST_WRITE_READ,
    LINC_GPIB_REQUEST_CLEAR,
} linc_gpib_request_type_t;

typedef struct
{
    linc_gpib_request_type_t type;

    uint8_t address;

    const uint8_t* tx_data;
    size_t tx_length;

    uint8_t* rx_data;
    size_t rx_capacity;
    size_t rx_length;

    bool send_eoi;
    ULONG timeout_ticks;

    linc_gpib_result_t result;

    TX_SEMAPHORE complete;
} linc_gpib_request_t;

UINT linc_gpib_create(TX_BYTE_POOL* byte_pool);

linc_gpib_status_t linc_gpib_get_status(void);

linc_gpib_result_t linc_gpib_write(uint8_t address, const uint8_t* data, size_t length, bool send_eoi);

linc_gpib_result_t linc_gpib_read(uint8_t address, uint8_t* data, size_t capacity, size_t* length);

linc_gpib_result_t linc_gpib_query(uint8_t address, const uint8_t* tx_data, size_t tx_length, uint8_t* rx_data,
                                   size_t rx_capacity, size_t* rx_length);

#endif // LINC_FIRMWARE_LINC_GPIB_H