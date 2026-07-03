//
// Created by Noah Husby on 7/2/26.
//

#include "linc_usb_cli.h"

#include <string.h>

#include "linc_usb_cli_commands.h"
#include "linc_usb_cli_parser.h"
#include "linc_usb_console.h"

static const LINC_USB_CLI_COMMAND* usb_cli_find_command(const LINC_USB_CLI_COMMAND* commands, UINT command_count,
                                                        const char* name);

static void usb_cli_execute(UINT argc, char* argv[]);
static void usb_cli_execute_help(UINT argc, char* argv[]);
static void usb_cli_print_command_list(const LINC_USB_CLI_COMMAND* commands, UINT command_count);
static void usb_cli_print_command_help(const LINC_USB_CLI_COMMAND* command);
static void usb_cli_print_unknown_command(const char* name);

static void usb_cli_print_banner(void)
{
    linc_usb_console_printf("\r\n"
                            "=====================================================\r\n"
                            " Laboratory Instrument Network Controller (LINC)\r\n"
                            " Designed by Husby Labs | Firmware v%s\r\n"
                            "\r\n"
                            " Type 'help' for available commands.\r\n"
                            "=====================================================\r\n"
                            "\r\n",
                            "0.2.0");
}

void linc_usb_cli_print_prompt(void) { linc_usb_console_write_string("> "); }

void linc_usb_cli_start(void)
{
    usb_cli_print_banner();
    linc_usb_cli_print_prompt();
}

void linc_usb_cli_process_line(char* line)
{
    UINT argc = 0;
    char* argv[LINC_USB_CLI_MAX_ARGS];

    LINC_USB_CLI_PARSE_RESULT parse_result = linc_usb_cli_parser_parse(line, &argc, argv);

    switch (parse_result)
    {
    case LINC_USB_CLI_PARSE_OK:
        break;

    case LINC_USB_CLI_PARSE_TOO_MANY_ARGS:
        linc_usb_console_write_string("Syntax error: too many arguments\r\n");
        linc_usb_cli_print_prompt();
        return;

    case LINC_USB_CLI_PARSE_UNTERMINATED_QUOTE:
        linc_usb_console_write_string("Syntax error: unterminated quote\r\n");
        linc_usb_cli_print_prompt();
        return;

    default:
        linc_usb_console_write_string("Syntax error\r\n");
        linc_usb_cli_print_prompt();
        return;
    }

    if (argc > 0)
    {
        usb_cli_execute(argc, argv);
    }

    linc_usb_cli_print_prompt();
}

static void usb_cli_execute(UINT argc, char* argv[])
{
    if (strcmp(argv[0], "help") == 0)
    {
        usb_cli_execute_help(argc, argv);
        return;
    }

    const LINC_USB_CLI_COMMAND* commands = linc_usb_cli_commands_get_root();

    UINT command_count = linc_usb_cli_commands_get_root_count();

    const LINC_USB_CLI_COMMAND* command = TX_NULL;

    UINT arg_index = 0;

    while (arg_index < argc)
    {
        command = usb_cli_find_command(commands, command_count, argv[arg_index]);

        if (command == TX_NULL)
        {
            usb_cli_print_unknown_command(argv[arg_index]);
            return;
        }

        arg_index++;

        if (command->children != TX_NULL && command->child_count > 0 && arg_index < argc)
        {
            commands = command->children;
            command_count = command->child_count;
            continue;
        }

        break;
    }

    if (command == TX_NULL)
    {
        return;
    }

    if (command->handler != TX_NULL)
    {
        command->handler(argc - arg_index, &argv[arg_index]);
        return;
    }

    if (command->children != TX_NULL && command->child_count > 0)
    {
        usb_cli_print_command_help(command);

        linc_usb_console_write_string("\r\nSubcommands:\r\n");

        usb_cli_print_command_list(command->children, command->child_count);

        return;
    }

    linc_usb_console_write_string("Command has no handler\r\n");
}

static void usb_cli_execute_help(UINT argc, char* argv[])
{
    const LINC_USB_CLI_COMMAND* commands = linc_usb_cli_commands_get_root();

    UINT command_count = linc_usb_cli_commands_get_root_count();

    const LINC_USB_CLI_COMMAND* command = TX_NULL;

    if (argc == 1)
    {
        linc_usb_console_write_string("Available commands:\r\n");

        usb_cli_print_command_list(commands, command_count);

        return;
    }

    for (UINT i = 1; i < argc; i++)
    {
        command = usb_cli_find_command(commands, command_count, argv[i]);

        if (command == TX_NULL)
        {
            usb_cli_print_unknown_command(argv[i]);
            return;
        }

        commands = command->children;
        command_count = command->child_count;
    }

    usb_cli_print_command_help(command);

    if (command->children != TX_NULL && command->child_count > 0)
    {
        linc_usb_console_write_string("\r\nSubcommands:\r\n");

        usb_cli_print_command_list(command->children, command->child_count);
    }
}

static const LINC_USB_CLI_COMMAND* usb_cli_find_command(const LINC_USB_CLI_COMMAND* commands, UINT command_count,
                                                        const char* name)
{
    if (commands == TX_NULL || name == TX_NULL)
    {
        return TX_NULL;
    }

    for (UINT i = 0; i < command_count; i++)
    {
        if (commands[i].name != TX_NULL && strcmp(commands[i].name, name) == 0)
        {
            return &commands[i];
        }
    }

    return TX_NULL;
}

static void usb_cli_print_command_list(const LINC_USB_CLI_COMMAND* commands, UINT command_count)
{
    linc_usb_console_begin_write();
    for (UINT i = 0; i < command_count; i++)
    {
        linc_usb_console_printf("  %-12s", commands[i].name);

        if (commands[i].description != TX_NULL)
        {
            linc_usb_console_printf("%s", commands[i].description);
        }

        linc_usb_console_write_string("\r\n");
    }
    linc_usb_console_end_write();
}

static void usb_cli_print_command_help(const LINC_USB_CLI_COMMAND* command)
{
    if (command == TX_NULL)
    {
        return;
    }

    linc_usb_console_printf("%s\r\n", command->name);

    if (command->description != TX_NULL)
    {
        linc_usb_console_printf("%s\r\n", command->description);
    }

    if (command->usage != TX_NULL)
    {
        linc_usb_console_write_string("\r\nUsage:\r\n");

        linc_usb_console_printf("  %s\r\n", command->usage);
    }
}

static void usb_cli_print_unknown_command(const char* name)
{
    linc_usb_console_printf("Unknown command: %s\r\n", name);

    linc_usb_console_write_string("Type \"help\" for available commands\r\n");
}