//
// Created by Noah Husby on 7/2/26.
//

#include "linc_usb_cli_commands.h"

#include <string.h>

#include "linc_gpib.h"
#include "main.h"

#include "linc_usb_console.h"

#define ARRAY_COUNT(x) ((UINT)(sizeof(x) / sizeof((x)[0])))

static UINT cmd_version(UINT argc, char* argv[]);
static UINT cmd_ping(UINT argc, char* argv[]);
static UINT cmd_usb_status(UINT argc, char* argv[]);
static UINT cmd_gpib_status(UINT argc, char* argv[]);
static UINT cmd_gpib_write(UINT argc, char* argv[]);
static UINT cmd_system_reboot(UINT argc, char* argv[]);
static UINT cmd_clear(UINT argc, char* argv[]);

static const LINC_USB_CLI_COMMAND usb_commands[] = {
    {
        .name = "status",
        .description = "Show USB status",
        .usage = "usb status",
        .handler = cmd_usb_status,
    },
};

static const LINC_USB_CLI_COMMAND gpib_commands[] = {
    {
        .name = "status",
        .description = "Show GPIB status",
        .usage = "gpib status",
        .handler = cmd_gpib_status,
    },
    {
        .name = "write",
        .description = "Write a command to the GPIB bus",
        .usage = "gpib write \"*IDN?\"",
        .handler = cmd_gpib_write,
    },
};

static const LINC_USB_CLI_COMMAND system_commands[] = {
    {
        .name = "reboot",
        .description = "Reboot the controller",
        .usage = "system reboot",
        .handler = cmd_system_reboot,
    },
};

static const LINC_USB_CLI_COMMAND root_commands[] = {
    {
        .name = "help",
        .description = "Show command help",
        .usage = "help [command] [subcommand]",
        .handler = TX_NULL,
    },
    {
        .name = "version",
        .description = "Show firmware version",
        .usage = "version",
        .handler = cmd_version,
    },
    {
        .name = "clear",
        .description = "Clear the console",
        .usage = "clear",
        .handler = cmd_clear,
    },
    {
        .name = "ping",
        .description = "Display a test message",
        .usage = "ping",
        .handler = cmd_ping,
    },
    {
        .name = "usb",
        .description = "USB commands",
        .usage = "usb <command>",
        .children = usb_commands,
        .child_count = ARRAY_COUNT(usb_commands),
    },
    {
        .name = "gpib",
        .description = "GPIB commands",
        .usage = "gpib <command>",
        .children = gpib_commands,
        .child_count = ARRAY_COUNT(gpib_commands),
    },
    {
        .name = "system",
        .description = "System commands",
        .usage = "system <command>",
        .children = system_commands,
        .child_count = ARRAY_COUNT(system_commands),
    },
};

const LINC_USB_CLI_COMMAND* linc_usb_cli_commands_get_root(void) { return root_commands; }

UINT linc_usb_cli_commands_get_root_count(void) { return ARRAY_COUNT(root_commands); }

static UINT cmd_version(UINT argc, char* argv[])
{
    (void)argc;
    (void)argv;

    linc_usb_console_write_string("LINC firmware\r\n");
    return TX_SUCCESS;
}

static UINT cmd_ping(UINT argc, char* argv[])
{

    (void)argc;
    (void)argv;

    linc_usb_console_write_string("Pong!\r\n");
    return TX_SUCCESS;
}

static UINT cmd_usb_status(UINT argc, char* argv[])
{
    (void)argc;
    (void)argv;

    linc_usb_console_write_string("USB status: TODO\r\n");
    return TX_SUCCESS;
}

static UINT cmd_gpib_status(UINT argc, char* argv[])
{
    (void)argc;
    (void)argv;

    linc_gpib_status_t status = linc_gpib_get_status();

    linc_usb_console_begin_write();
    linc_usb_console_write_string("GPIB Bus Status\r\n");
    linc_usb_console_write_string("------------------------------\r\n");

    linc_usb_console_printf(
        "  SRQ  (Service Request)      : %s\r\n",
        status.srq ? "ASSERTED" : "RELEASED");

    linc_usb_console_printf(
        "  NDAC (Not Data Accepted)    : %s\r\n",
        status.ndac ? "ASSERTED" : "RELEASED");

    linc_usb_console_printf(
        "  NRFD (Not Ready For Data)   : %s\r\n",
        status.nrfd ? "ASSERTED" : "RELEASED");

    linc_usb_console_printf(
        "  DAV  (Data Valid)           : %s\r\n",
        status.dav ? "ASSERTED" : "RELEASED");

    linc_usb_console_printf(
        "  EOI  (End Or Identify)      : %s\r\n",
        status.eoi ? "ASSERTED" : "RELEASED");
    linc_usb_console_end_write();
    return TX_SUCCESS;
}

static UINT cmd_gpib_write(UINT argc, char* argv[])
{
    if (argc != 1)
    {
        linc_usb_console_write_string("Usage: gpib write \"command\"\r\n");
        return UX_ERROR;
    }

    linc_usb_console_printf("GPIB write: %s\r\n", argv[0]);
    return TX_SUCCESS;
}

static UINT cmd_system_reboot(UINT argc, char* argv[])
{
    (void)argc;
    (void)argv;

    HAL_NVIC_SystemReset();
    return TX_SUCCESS;
}

static UINT cmd_clear(UINT argc, char* argv[]) {
    (void)argc;
    (void)argv;

    linc_usb_console_clear();

    return TX_SUCCESS;
}