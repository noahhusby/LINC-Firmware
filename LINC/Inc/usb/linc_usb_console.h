//
// Created by Noah Husby on 7/2/26.
//

#pragma once

#ifndef LINC_USB_CONSOLE_H
#define LINC_USB_CONSOLE_H

#include <stdarg.h>

#include "ux_api.h"

void linc_usb_console_init(void);

UINT linc_usb_console_write(const void* buffer, ULONG length);

UINT linc_usb_console_write_string(const char* string);

UINT linc_usb_console_printf(const char* format, ...);

UINT linc_usb_console_read(void* buffer, ULONG buffer_length, ULONG* actual_length);

void linc_console_receive(const UCHAR* data, ULONG length);
#endif