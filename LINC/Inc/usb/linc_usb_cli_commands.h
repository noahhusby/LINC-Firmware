//
// Created by Noah Husby on 7/2/26.
//

#ifndef LINC_USB_CLI_COMMANDS_H
#define LINC_USB_CLI_COMMANDS_H

#include "linc_usb_cli.h"
#include "tx_api.h"

const LINC_USB_CLI_COMMAND* linc_usb_cli_commands_get_root(void);
UINT linc_usb_cli_commands_get_root_count(void);

#endif