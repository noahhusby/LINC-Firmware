//
// Created by Noah Husby on 7/2/26.
//

#include "linc_usb_console.h"

#include "linc_usb.h"
#define LINC_USB_CONSOLE_PRINTF_BUFFER_SIZE 256
#define LINC_CONSOLE_LINE_BUFFER_SIZE 128
static char line_buffer[LINC_CONSOLE_LINE_BUFFER_SIZE];
static ULONG line_length = 0;
void linc_usb_console_init(void) {}

UINT linc_usb_console_write(const void* buffer, ULONG length)
{
    return linc_usb_write(LINC_USB_ENDPOINT_CONSOLE, buffer, length);
}

UINT linc_usb_console_write_string(const char* string) { return linc_usb_console_write(string, (ULONG)strlen(string)); }

UINT linc_usb_console_printf(const char* format, ...)
{
    char buffer[LINC_USB_CONSOLE_PRINTF_BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (length < 0)
    {
        return UX_ERROR;
    }
    if (length > sizeof(buffer))
    {
        length = sizeof(buffer);
    }

    return linc_usb_console_write(buffer, (ULONG)length);
}

UINT linc_usb_console_read(void* buffer, ULONG buffer_length, ULONG* actual_length)
{
    if (!linc_usb_connected())
    {
        return UX_ERROR;
    }

    return ux_device_class_cdc_acm_read(linc_usb_cdc(), buffer, buffer_length, actual_length);
}

void linc_console_receive(const UCHAR* data, ULONG length)
{
    for (ULONG i = 0; i < length; i++)
    {
        char c = (char)data[i];

        switch (c)
        {
        case '\r':
        case '\n':
        {
            if (line_length == 0)
            {
                linc_usb_console_write_string("> ");
                break;
            }

            line_buffer[line_length] = '\0';

            linc_usb_console_printf("\r\nReceived: %s\r\n> ", line_buffer);

            line_length = 0;
            break;
        }

        case '\b':
        case 0x7F:
        {
            if (line_length > 0)
            {
                line_length--;
            }
            break;
        }

        default:
        {
            if (line_length < (LINC_CONSOLE_LINE_BUFFER_SIZE - 1))
            {
                line_buffer[line_length++] = c;
            }
            break;
        }
        }
    }
}