#include "ux_device_usbtmc.h"

static UINT ux_device_usbtmc_initialize(UX_SLAVE_CLASS_COMMAND *command);
static UINT ux_device_usbtmc_uninitialize(UX_SLAVE_CLASS_COMMAND *command);
static UINT ux_device_usbtmc_activate(UX_SLAVE_CLASS_COMMAND *command);
static UINT ux_device_usbtmc_deactivate(UX_SLAVE_CLASS_COMMAND *command);
static UINT ux_device_usbtmc_control_request(UX_SLAVE_CLASS_COMMAND *command);


UINT ux_device_usbtmc_entry(UX_SLAVE_CLASS_COMMAND *command)
{
    switch (command->ux_slave_class_command_request)
    {
        case UX_SLAVE_CLASS_COMMAND_INITIALIZE:
            return ux_device_usbtmc_initialize(command);

        case UX_SLAVE_CLASS_COMMAND_UNINITIALIZE:
            return ux_device_usbtmc_uninitialize(command);

        case UX_SLAVE_CLASS_COMMAND_ACTIVATE:
            return ux_device_usbtmc_activate(command);

        case UX_SLAVE_CLASS_COMMAND_DEACTIVATE:
            return ux_device_usbtmc_deactivate(command);

        case UX_SLAVE_CLASS_COMMAND_REQUEST:
            return ux_device_usbtmc_control_request(command);

        default:
            return UX_FUNCTION_NOT_SUPPORTED;
    }
}


static UINT ux_device_usbtmc_initialize(UX_SLAVE_CLASS_COMMAND *command)
{
    UX_SLAVE_CLASS *class_ptr;
    UX_DEVICE_USBTMC *usbtmc;

    class_ptr = command->ux_slave_class_command_class_ptr;

    usbtmc = _ux_utility_memory_allocate(UX_NO_ALIGN,
                                         UX_REGULAR_MEMORY,
                                         sizeof(UX_DEVICE_USBTMC));

    if (usbtmc == UX_NULL)
    {
        return UX_MEMORY_INSUFFICIENT;
    }

    _ux_utility_memory_set(usbtmc, 0, sizeof(UX_DEVICE_USBTMC));

    class_ptr->ux_slave_class_instance = usbtmc;

    return UX_SUCCESS;
}


static UINT ux_device_usbtmc_uninitialize(UX_SLAVE_CLASS_COMMAND *command)
{
    UX_SLAVE_CLASS *class_ptr;
    UX_DEVICE_USBTMC *usbtmc;

    class_ptr = command->ux_slave_class_command_class_ptr;
    usbtmc = (UX_DEVICE_USBTMC *)class_ptr->ux_slave_class_instance;

    if (usbtmc != UX_NULL)
    {
        _ux_utility_memory_free(usbtmc);
        class_ptr->ux_slave_class_instance = UX_NULL;
    }

    return UX_SUCCESS;
}


static UINT ux_device_usbtmc_activate(UX_SLAVE_CLASS_COMMAND *command)
{
    UX_SLAVE_CLASS *class_ptr;
    UX_SLAVE_INTERFACE *interface;
    UX_SLAVE_ENDPOINT *endpoint;
    UX_DEVICE_USBTMC *usbtmc;

    class_ptr = command->ux_slave_class_command_class_ptr;
    interface = command->ux_slave_class_command_interface;
    usbtmc = (UX_DEVICE_USBTMC *)class_ptr->ux_slave_class_instance;

    if ((interface == UX_NULL) || (usbtmc == UX_NULL))
    {
        return UX_ERROR;
    }

    usbtmc->interface = interface;
    usbtmc->bulk_in_endpoint = UX_NULL;
    usbtmc->bulk_out_endpoint = UX_NULL;

    interface->ux_slave_interface_class_instance = usbtmc;

    endpoint = interface->ux_slave_interface_first_endpoint;

    while (endpoint != UX_NULL)
    {
        if ((endpoint->ux_slave_endpoint_descriptor.bmAttributes & UX_MASK_ENDPOINT_TYPE) ==
            UX_BULK_ENDPOINT)
        {
            if ((endpoint->ux_slave_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) ==
                UX_ENDPOINT_IN)
            {
                usbtmc->bulk_in_endpoint = endpoint;
            }
            else
            {
                usbtmc->bulk_out_endpoint = endpoint;
            }
        }

        endpoint = endpoint->ux_slave_endpoint_next_endpoint;
    }

    if ((usbtmc->bulk_in_endpoint == UX_NULL) ||
        (usbtmc->bulk_out_endpoint == UX_NULL))
    {
        return UX_ERROR;
    }

    return UX_SUCCESS;
}


static UINT ux_device_usbtmc_deactivate(UX_SLAVE_CLASS_COMMAND *command)
{
    UX_SLAVE_CLASS *class_ptr;
    UX_DEVICE_USBTMC *usbtmc;

    class_ptr = command->ux_slave_class_command_class_ptr;
    usbtmc = (UX_DEVICE_USBTMC *)class_ptr->ux_slave_class_instance;

    if (usbtmc != UX_NULL)
    {
        if (usbtmc->interface != UX_NULL)
        {
            usbtmc->interface->ux_slave_interface_class_instance = UX_NULL;
        }

        usbtmc->interface = UX_NULL;
        usbtmc->bulk_in_endpoint = UX_NULL;
        usbtmc->bulk_out_endpoint = UX_NULL;
    }

    return UX_SUCCESS;
}


static UINT ux_device_usbtmc_control_request(UX_SLAVE_CLASS_COMMAND *command)
{
    UX_PARAMETER_NOT_USED(command);

    /*
     * USBTMC class-specific control requests will be implemented here later.
     */
    return UX_FUNCTION_NOT_SUPPORTED;
}


UINT ux_device_usbtmc_read(UX_DEVICE_USBTMC *usbtmc,
                           UCHAR *buffer,
                           ULONG requested_length,
                           ULONG *actual_length)
{
    UX_SLAVE_TRANSFER *transfer;
    UINT status;

    if ((usbtmc == UX_NULL) ||
        (buffer == UX_NULL) ||
        (actual_length == UX_NULL) ||
        (usbtmc->bulk_out_endpoint == UX_NULL))
    {
        return UX_INVALID_PARAMETER;
    }

    transfer = &usbtmc->bulk_out_endpoint->ux_slave_endpoint_transfer_request;

    status = _ux_device_stack_transfer_request(transfer,
                                               requested_length,
                                               requested_length);

    if (status != UX_SUCCESS)
    {
        *actual_length = 0;
        return status;
    }

    *actual_length = transfer->ux_slave_transfer_request_actual_length;

    _ux_utility_memory_copy(buffer,
                            transfer->ux_slave_transfer_request_data_pointer,
                            *actual_length);

    return UX_SUCCESS;
}


UINT ux_device_usbtmc_write(UX_DEVICE_USBTMC *usbtmc,
                            const UCHAR *buffer,
                            ULONG requested_length,
                            ULONG *actual_length)
{
    UX_SLAVE_TRANSFER *transfer;
    UINT status;

    if ((usbtmc == UX_NULL) ||
        (buffer == UX_NULL) ||
        (actual_length == UX_NULL) ||
        (usbtmc->bulk_in_endpoint == UX_NULL))
    {
        return UX_INVALID_PARAMETER;
    }

    transfer = &usbtmc->bulk_in_endpoint->ux_slave_endpoint_transfer_request;

    _ux_utility_memory_copy(transfer->ux_slave_transfer_request_data_pointer,
                            buffer,
                            requested_length);

    status = _ux_device_stack_transfer_request(transfer,
                                               requested_length,
                                               requested_length);

    if (status != UX_SUCCESS)
    {
        *actual_length = 0;
        return status;
    }

    *actual_length = transfer->ux_slave_transfer_request_actual_length;

    return UX_SUCCESS;
}
