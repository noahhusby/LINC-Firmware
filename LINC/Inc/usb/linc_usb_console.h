//
// Created by Noah Husby on 7/2/26.
//

#pragma once

#ifndef LINC_USB_CONSOLE_H
#define LINC_USB_CONSOLE_H

#include <stdarg.h>

#define LINC_USB_CONSOLE_PRINTF_BUFFER_SIZE 256
#define LINC_USB_CONSOLE_WRITE_BUFFER_SIZE 4096

#define LINC_USB_CONSOLE_HISTORY_SIZE 8

#include "ux_api.h"

typedef struct
{
    char entries[LINC_USB_CONSOLE_HISTORY_SIZE][LINC_USB_CONSOLE_PRINTF_BUFFER_SIZE];

    UINT count;
    UINT head;
    INT index;
} linc_usb_console_history_t;

void linc_usb_console_init(void);

UINT linc_usb_console_write(const void* buffer, ULONG length);
UINT linc_usb_console_write_string(const char* string);
UINT linc_usb_console_printf(const char* format, ...);

UINT linc_usb_console_read(void* buffer, ULONG buffer_length, ULONG* actual_length);

void linc_usb_console_process_input(const UCHAR* data, ULONG length);

void linc_usb_console_connected(void);
void linc_usb_console_disconnected(void);

void linc_usb_console_begin_write(void);
UINT linc_usb_console_end_write(void);

void linc_usb_console_clear(void);

#endif