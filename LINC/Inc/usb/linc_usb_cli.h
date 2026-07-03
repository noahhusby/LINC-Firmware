//
// Created by Noah Husby on 7/2/26.
//

#ifndef LINC_USB_CLI_H
#define LINC_USB_CLI_H

#include "tx_api.h"

#define LINC_USB_CLI_MAX_ARGS 16
#define LINC_USB_CLI_MAX_LINE_LENGTH 256

typedef UINT (*LINC_USB_CLI_HANDLER)(UINT argc, char* argv[]);

typedef struct LINC_USB_CLI_COMMAND
{
    const char* name;
    const char* description;
    const char* usage;

    LINC_USB_CLI_HANDLER handler;

    const struct LINC_USB_CLI_COMMAND* children;
    UINT child_count;
} LINC_USB_CLI_COMMAND;

void linc_usb_cli_init(void);
void linc_usb_cli_process_line(char* line);
void linc_usb_cli_print_prompt(void);

#endif