//
// Created by Noah Husby on 7/2/26.
//

#include "usb/linc_usb_cli.h"

#include <stdbool.h>
#include <string.h>

#include "usb/linc_usb_cli_commands.h"
#include "usb/linc_usb_console.h"

typedef enum
{
    USB_CLI_PARSE_OK = 0,
    USB_CLI_PARSE_TOO_MANY_ARGS,
    USB_CLI_PARSE_UNTERMINATED_QUOTE,
} USB_CLI_PARSE_RESULT;

static USB_CLI_PARSE_RESULT usb_cli_tokenize(char* line, UINT* argc, char* argv[]);
static const LINC_USB_CLI_COMMAND* usb_cli_find_command(const LINC_USB_CLI_COMMAND* commands, UINT command_count,
                                                        const char* name);

static void usb_cli_execute(UINT argc, char* argv[]);
static void usb_cli_execute_help(UINT argc, char* argv[]);
static void usb_cli_print_command_list(const LINC_USB_CLI_COMMAND* commands, UINT command_count);
static void usb_cli_print_command_help(const LINC_USB_CLI_COMMAND* command);
static void usb_cli_print_unknown_command(const char* name);

static const LINC_USB_CLI_COMMAND* usb_cli_root_commands(void) { return linc_usb_cli_commands_get_root(); }

static UINT usb_cli_root_command_count(void) { return linc_usb_cli_commands_get_root_count(); }

void linc_usb_cli_print_prompt(void) { linc_usb_console_write_string("> "); }

void linc_usb_cli_process_line(char* line)
{
    UINT argc = 0;
    char* argv[LINC_USB_CLI_MAX_ARGS];

    USB_CLI_PARSE_RESULT parse_result = usb_cli_tokenize(line, &argc, argv);

    switch (parse_result)
    {
    case USB_CLI_PARSE_OK:
        break;

    case USB_CLI_PARSE_TOO_MANY_ARGS:
        linc_usb_console_write_string("Syntax error: too many arguments\r\n");
        linc_usb_cli_print_prompt();
        return;

    case USB_CLI_PARSE_UNTERMINATED_QUOTE:
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

static USB_CLI_PARSE_RESULT usb_cli_tokenize(char* line, UINT* argc, char* argv[])
{
    char* read = line;
    char* write = line;

    *argc = 0;

    while (*read != '\0')
    {
        while (*read == ' ' || *read == '\t' || *read == '\r' || *read == '\n')
        {
            read++;
        }

        if (*read == '\0')
        {
            break;
        }

        if (*argc >= LINC_USB_CLI_MAX_ARGS)
        {
            return USB_CLI_PARSE_TOO_MANY_ARGS;
        }

        argv[*argc] = write;
        (*argc)++;

        bool in_quotes = false;
        bool closed_quote = false;

        if (*read == '"')
        {
            in_quotes = true;
            read++;
        }

        while (*read != '\0')
        {
            if (in_quotes)
            {
                if (*read == '"')
                {
                    closed_quote = true;
                    read++;
                    break;
                }
            }
            else
            {
                if (*read == ' ' || *read == '\t' || *read == '\r' || *read == '\n')
                {
                    break;
                }
            }

            if (*read == '\\')
            {
                read++;

                switch (*read)
                {
                case '\0':
                    *write++ = '\\';
                    break;

                case 'n':
                    *write++ = '\n';
                    read++;
                    break;

                case 'r':
                    *write++ = '\r';
                    read++;
                    break;

                case 't':
                    *write++ = '\t';
                    read++;
                    break;

                case '"':
                    *write++ = '"';
                    read++;
                    break;

                case '\\':
                    *write++ = '\\';
                    read++;
                    break;

                default:
                    *write++ = *read++;
                    break;
                }
            }
            else
            {
                *write++ = *read++;
            }
        }

        if (in_quotes && !closed_quote)
        {
            return USB_CLI_PARSE_UNTERMINATED_QUOTE;
        }

        *write++ = '\0';

        while (*read == ' ' || *read == '\t' || *read == '\r' || *read == '\n')
        {
            read++;
        }
    }

    return USB_CLI_PARSE_OK;
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
        usb_cli_print_command_list(linc_usb_cli_commands_get_root(), linc_usb_cli_commands_get_root_count());
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
    for (UINT i = 0; i < command_count; i++)
    {
        linc_usb_console_printf("  %-12s", commands[i].name);

        if (commands[i].description != TX_NULL)
        {
            linc_usb_console_printf("%s", commands[i].description);
        }

        linc_usb_console_write_string("\r\n");
    }
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