//
// Created by Noah Husby on 6/11/26.
//

#pragma once

#ifndef LINC_USB_H
#define LINC_USB_H

#include <stdbool.h>

#include "ux_api.h"
#include "tx_api.h"
#include "ux_device_class_cdc_acm.h"

#define LINC_USB_THREAD_STACK_SIZE 2048
#define LINC_USB_THREAD_PRIORITY 9

#define LINC_USB_MAX_PACKET_SIZE 256
#define LINC_USB_TX_QUEUE_DEPTH 8
#define LINC_USB_RX_QUEUE_DEPTH 8

typedef enum
{
    LINC_USB_ENDPOINT_CONSOLE = 0,
    LINC_USB_ENDPOINT_VENDOR,
} linc_usb_endpoint_t;

typedef struct
{
    linc_usb_endpoint_t endpoint;
    ULONG length;
    UCHAR data[LINC_USB_MAX_PACKET_SIZE];
} linc_usb_packet_t;

typedef struct
{
    bool connected;
    UX_SLAVE_CLASS_CDC_ACM* cdc;

    TX_THREAD tx_thread;
    TX_THREAD rx_thread;

    TX_QUEUE tx_queue;
    TX_QUEUE rx_queue;

    VOID* tx_stack;
    VOID* rx_stack;
} linc_usb_t;

void linc_usb_init(void);

UINT linc_usb_create(TX_BYTE_POOL* byte_pool);

bool linc_usb_connected(void);

void linc_usb_activate(UX_SLAVE_CLASS_CDC_ACM* cdc);

void linc_usb_deactivate(void);

UINT linc_usb_write(linc_usb_endpoint_t endpoint, const void* buffer, ULONG length);

UX_SLAVE_CLASS_CDC_ACM* linc_usb_cdc(void);

VOID linc_usb_tx_thread_entry(ULONG thread_input);
VOID linc_usb_rx_thread_entry(ULONG thread_input);

#endif