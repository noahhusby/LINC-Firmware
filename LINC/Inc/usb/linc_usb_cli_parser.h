//
// Created by Noah Husby on 7/2/26.
//

#ifndef LINC_USB_CLI_PARSER_H
#define LINC_USB_CLI_PARSER_H

#include "tx_api.h"

#define LINC_USB_CLI_MAX_ARGS 16

typedef enum
{
    LINC_USB_CLI_PARSE_OK = 0,
    LINC_USB_CLI_PARSE_TOO_MANY_ARGS,
    LINC_USB_CLI_PARSE_UNTERMINATED_QUOTE,
} LINC_USB_CLI_PARSE_RESULT;

LINC_USB_CLI_PARSE_RESULT linc_usb_cli_parser_parse(char* line, UINT* argc, char* argv[]);

#endif