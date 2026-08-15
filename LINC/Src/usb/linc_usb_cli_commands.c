//
// Created by Noah Husby on 7/2/26.
//

#include "linc_usb_cli_commands.h"

#include <string.h>

#include "linc_gpib.h"
#include "main.h"

#include "linc_usb_console.h"

#define ARRAY_COUNT(x) ((UINT)(sizeof(x) / sizeof((x)[0])))
#define LINC_USB_CLI_GPIB_BUFFER_SIZE 256

static UINT cmd_version(UINT argc, char* argv[]);
static UINT cmd_ping(UINT argc, char* argv[]);
static UINT cmd_usb_status(UINT argc, char* argv[]);
static UINT cmd_gpib_status(UINT argc, char* argv[]);
static UINT cmd_gpib_write(UINT argc, char* argv[]);
static UINT cmd_gpib_read(UINT argc, char* argv[]);
static UINT cmd_gpib_query(UINT argc, char* argv[]);
static UINT cmd_system_reboot(UINT argc, char* argv[]);
static UINT cmd_clear(UINT argc, char* argv[]);

static bool parse_gpib_address(const char* string, uint8_t* address);
static const char* gpib_result_string(linc_gpib_result_t result);

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
        .description = "Show GPIB bus status",
        .usage = "gpib status",
        .handler = cmd_gpib_status,
    },
    {
        .name = "write",
        .description = "Write data to a GPIB instrument",
        .usage = "gpib write <address> \"command\"",
        .handler = cmd_gpib_write,
    },
    {
        .name = "read",
        .description = "Read data from a GPIB instrument",
        .usage = "gpib read <address>",
        .handler = cmd_gpib_read,
    },
    {
        .name = "query",
        .description = "Write a command and read the response",
        .usage = "gpib query <address> \"command\"",
        .handler = cmd_gpib_query,
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

    linc_usb_console_printf("  SRQ  (Service Request)      : %s\r\n", status.srq ? "ASSERTED" : "RELEASED");

    linc_usb_console_printf("  NDAC (Not Data Accepted)    : %s\r\n", status.ndac ? "ASSERTED" : "RELEASED");

    linc_usb_console_printf("  NRFD (Not Ready For Data)   : %s\r\n", status.nrfd ? "ASSERTED" : "RELEASED");

    linc_usb_console_printf("  DAV  (Data Valid)           : %s\r\n", status.dav ? "ASSERTED" : "RELEASED");

    linc_usb_console_printf("  EOI  (End Or Identify)      : %s\r\n", status.eoi ? "ASSERTED" : "RELEASED");
    linc_usb_console_end_write();
    return TX_SUCCESS;
}

static UINT cmd_gpib_write(UINT argc, char* argv[])
{
    if (argc != 2)
    {
        linc_usb_console_write_string("Usage: gpib write <address> \"command\"\r\n");

        return UX_ERROR;
    }

    uint8_t address;

    if (!parse_gpib_address(argv[0], &address))
    {
        linc_usb_console_write_string("Invalid GPIB address. Valid range is 0-30.\r\n");

        return UX_ERROR;
    }

    const char* data = argv[1];

    linc_gpib_result_t result = linc_gpib_write(address, (const uint8_t*)data, strlen(data), true);

    if (result != LINC_GPIB_OK)
    {
        linc_usb_console_printf("GPIB write failed: %s\r\n", gpib_result_string(result));

        return UX_ERROR;
    }

    linc_usb_console_write_string("GPIB write complete.\r\n");

    return TX_SUCCESS;
}

static UINT cmd_gpib_read(UINT argc, char* argv[])
{
    if (argc != 1)
    {
        linc_usb_console_write_string("Usage: gpib read <address>\r\n");

        return UX_ERROR;
    }

    uint8_t address;

    if (!parse_gpib_address(argv[0], &address))
    {
        linc_usb_console_write_string("Invalid GPIB address. Valid range is 0-30.\r\n");

        return UX_ERROR;
    }

    uint8_t buffer[LINC_USB_CLI_GPIB_BUFFER_SIZE];
    size_t length = 0;

    linc_gpib_result_t result = linc_gpib_read(address, buffer, sizeof(buffer) - 1, &length);

    if (result != LINC_GPIB_OK)
    {
        linc_usb_console_printf("GPIB read failed: %s\r\n", gpib_result_string(result));

        return UX_ERROR;
    }

    /*
     * Leave room above for this terminator.
     */
    buffer[length] = '\0';

    linc_usb_console_printf("GPIB read (%u bytes): %s\r\n", (unsigned int)length, (char*)buffer);

    return TX_SUCCESS;
}

static UINT cmd_gpib_query(UINT argc, char* argv[])
{
    if (argc != 2)
    {
        linc_usb_console_write_string("Usage: gpib query <address> \"command\"\r\n");

        return UX_ERROR;
    }

    uint8_t address;

    if (!parse_gpib_address(argv[0], &address))
    {
        linc_usb_console_write_string("Invalid GPIB address. Valid range is 0-30.\r\n");

        return UX_ERROR;
    }

    const char* command = argv[1];

    uint8_t buffer[LINC_USB_CLI_GPIB_BUFFER_SIZE];
    size_t length = 0;

    linc_gpib_result_t result =
        linc_gpib_query(address, (const uint8_t*)command, strlen(command), buffer, sizeof(buffer) - 1, &length);

    if (result != LINC_GPIB_OK)
    {
        linc_usb_console_printf("GPIB query failed: %s\r\n", gpib_result_string(result));

        return UX_ERROR;
    }

    buffer[length] = '\0';

    linc_usb_console_begin_write();

    linc_usb_console_printf("GPIB response (%u bytes):\r\n", (unsigned int)length);

    linc_usb_console_write_string((char*)buffer);
    linc_usb_console_write_string("\r\n");

    linc_usb_console_end_write();

    return TX_SUCCESS;
}

static UINT cmd_system_reboot(UINT argc, char* argv[])
{
    (void)argc;
    (void)argv;

    HAL_NVIC_SystemReset();
    return TX_SUCCESS;
}

static UINT cmd_clear(UINT argc, char* argv[])
{
    (void)argc;
    (void)argv;

    linc_usb_console_clear();

    return TX_SUCCESS;
}

static bool parse_gpib_address(const char* string, uint8_t* address)
{
    if (string == NULL || address == NULL)
    {
        return false;
    }

    char* end;

    unsigned long value = strtoul(string, &end, 10);

    if (*string == '\0' || *end != '\0')
    {
        return false;
    }

    /*
     * Primary GPIB addresses are 0-30.
     */
    if (value > 30)
    {
        return false;
    }

    *address = (uint8_t)value;
    return true;
}

static const char* gpib_result_string(linc_gpib_result_t result)
{
    switch (result)
    {
    case LINC_GPIB_OK:
        return "OK";

    case LINC_GPIB_ERROR:
        return "ERROR";

    case LINC_GPIB_TIMEOUT:
        return "TIMEOUT";

    case LINC_GPIB_INVALID_ARGUMENT:
        return "INVALID ARGUMENT";

    case LINC_GPIB_BUFFER_TOO_SMALL:
        return "BUFFER TOO SMALL";

    default:
        return "UNKNOWN";
    }
}