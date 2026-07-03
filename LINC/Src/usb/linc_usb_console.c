//
// Created by Noah Husby on 7/2/26.
//

#include "linc_usb_console.h"

#include "linc_usb.h"
#include "linc_usb_cli.h"
static char line_buffer[LINC_USB_CONSOLE_PRINTF_BUFFER_SIZE];
static ULONG line_length = 0;

static linc_usb_console_history_t history = {0};

static char write_buffer[LINC_USB_CONSOLE_WRITE_BUFFER_SIZE];
static ULONG write_length = 0;
static bool write_active = false;

static void linc_usb_console_history_push(const char* command);
static void linc_usb_console_history_previous(void);
static void linc_usb_console_history_next(void);
static void linc_usb_console_replace_line(const char* line);

void linc_usb_console_init(void) {}

void linc_usb_console_begin_write(void)
{
    write_length = 0;
    write_active = true;
}

UINT linc_usb_console_end_write(void)
{
    if (!write_active)
    {
        return UX_SUCCESS;
    }

    write_active = false;

    ULONG offset = 0;

    while (offset < write_length)
    {
        ULONG chunk_length = write_length - offset;

        if (chunk_length > LINC_USB_TX_BUFFER_SIZE)
        {
            chunk_length = LINC_USB_TX_BUFFER_SIZE;
        }

        UINT status = linc_usb_write(LINC_USB_ENDPOINT_CONSOLE, &write_buffer[offset], chunk_length);

        if (status != UX_SUCCESS)
        {
            write_length = 0;
            return status;
        }

        offset += chunk_length;
    }

    write_length = 0;

    return UX_SUCCESS;
}

UINT linc_usb_console_write(const void* buffer, ULONG length)
{
    if (!write_active)
    {
        return linc_usb_write(LINC_USB_ENDPOINT_CONSOLE, buffer, length);
    }

    if ((write_length + length) > LINC_USB_CONSOLE_WRITE_BUFFER_SIZE)
    {
        return UX_ERROR;
    }

    memcpy(&write_buffer[write_length], buffer, length);

    write_length += length;

    return UX_SUCCESS;
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

    if ((size_t)length >= sizeof(buffer))
    {
        length = sizeof(buffer) - 1;
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

void linc_usb_console_connected(void)
{
    line_length = 0;
    linc_usb_cli_start();
}

void linc_usb_console_disconnected(void) { line_length = 0; }

void linc_usb_console_process_input(const UCHAR* data, ULONG length)
{
    for (ULONG i = 0; i < length; i++)
    {
        char c = (char)data[i];

        switch (c)
        {
        //
        // ENTER
        //
        case '\r':
        case '\n':
        {
            /* Ignore the LF of a CRLF sequence */
            if (c == '\n')
            {
                continue;
            }

            linc_usb_console_write_string("\r\n");

            if (line_length == 0)
            {
                linc_usb_cli_print_prompt();
                break;
            }

            line_buffer[line_length] = '\0';
            linc_usb_console_history_push(line_buffer);
            linc_usb_cli_process_line(line_buffer);

            line_length = 0;

            break;
        }

            //
            // BACKSPACE
            //
        case '\b':
        case 0x7F:
        {
            if (line_length > 0)
            {
                line_length--;

                /* Erase the character on the user's terminal */
                linc_usb_console_write_string("\b \b");
            }

            break;
        }

        case 0x1B:
        {
            if ((i + 2) < length && data[i + 1] == '[')
            {
                switch (data[i + 2])
                {
                case 'A':
                    linc_usb_console_history_previous();
                    break;

                case 'B':
                    linc_usb_console_history_next();
                    break;

                default:
                    break;
                }

                i += 2;
            }

            break;
        }

            //
            // Printable ASCII
            //
        default:
        {
            if ((c >= 32) && (c <= 126))
            {
                if (line_length < (LINC_USB_CONSOLE_PRINTF_BUFFER_SIZE - 1))
                {
                    line_buffer[line_length++] = c;

                    /* Immediate echo */
                    linc_usb_console_write(&c, 1);
                }
            }

            break;
        }
        }
    }
}

static void linc_usb_console_replace_line(const char* line)
{
    while (line_length > 0)
    {
        linc_usb_console_write_string("\b \b");
        line_length--;
    }

    strncpy(line_buffer, line, sizeof(line_buffer) - 1);

    line_buffer[sizeof(line_buffer) - 1] = '\0';

    line_length = strlen(line_buffer);

    linc_usb_console_write(line_buffer, line_length);
}

static void linc_usb_console_history_push(const char* command)
{
    if (command[0] == '\0')
    {
        return;
    }

    if (history.count > 0)
    {
        UINT newest = (history.head + LINC_USB_CONSOLE_HISTORY_SIZE - 1) % LINC_USB_CONSOLE_HISTORY_SIZE;

        if (strcmp(history.entries[newest], command) == 0)
        {
            history.index = -1;
            return;
        }
    }

    strncpy(history.entries[history.head], command, LINC_USB_CONSOLE_PRINTF_BUFFER_SIZE - 1);

    history.entries[history.head][LINC_USB_CONSOLE_PRINTF_BUFFER_SIZE - 1] = '\0';

    history.head = (history.head + 1) % LINC_USB_CONSOLE_HISTORY_SIZE;

    if (history.count < LINC_USB_CONSOLE_HISTORY_SIZE)
    {
        history.count++;
    }

    history.index = -1;
}

static void linc_usb_console_history_previous(void)
{
    if (history.count == 0)
    {
        return;
    }

    if (history.index < (INT)history.count - 1)
    {
        history.index++;
    }

    UINT slot = (history.head + LINC_USB_CONSOLE_HISTORY_SIZE - 1 - history.index) % LINC_USB_CONSOLE_HISTORY_SIZE;

    linc_usb_console_replace_line(history.entries[slot]);
}

static void linc_usb_console_history_next(void)
{
    if (history.count == 0)
    {
        return;
    }

    if (history.index <= 0)
    {
        history.index = -1;
        linc_usb_console_replace_line("");
        return;
    }

    history.index--;

    UINT slot = (history.head + LINC_USB_CONSOLE_HISTORY_SIZE - 1 - history.index) % LINC_USB_CONSOLE_HISTORY_SIZE;

    linc_usb_console_replace_line(history.entries[slot]);
}