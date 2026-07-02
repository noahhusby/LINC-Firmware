//
// Created by Noah Husby on 6/18/26.
//

#include "linc_usb.h"

static linc_usb_t usb;

void linc_usb_init(void)
{
    memset(&usb, 0, sizeof(usb));
    usb.connected = false;
    usb.cdc = UX_NULL;
}

UINT linc_usb_create(TX_BYTE_POOL* byte_pool)
{
    UINT status = tx_byte_allocate(byte_pool, &usb.tx_stack, LINC_USB_THREAD_STACK_SIZE, TX_NO_WAIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_byte_allocate(byte_pool, &usb.rx_stack, LINC_USB_THREAD_STACK_SIZE, TX_NO_WAIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_thread_create(&usb.tx_thread, "USB TX", linc_usb_tx_thread_entry, 0, usb.tx_stack,
                              LINC_USB_THREAD_STACK_SIZE, LINC_USB_THREAD_PRIORITY, LINC_USB_THREAD_PRIORITY,
                              TX_NO_TIME_SLICE, TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_thread_create(&usb.rx_thread, "USB RX", linc_usb_rx_thread_entry, 0, usb.rx_stack,
                              LINC_USB_THREAD_STACK_SIZE, LINC_USB_THREAD_PRIORITY, LINC_USB_THREAD_PRIORITY,
                              TX_NO_TIME_SLICE, TX_AUTO_START);
    return status;
}

bool linc_usb_connected(void) { return usb.connected; }

void linc_usb_activate(UX_SLAVE_CLASS_CDC_ACM* cdc)
{
    usb.cdc = cdc;
    usb.connected = true;
}

void linc_usb_deactivate(void)
{
    usb.connected = false;
    usb.cdc = UX_NULL;
}

UINT linc_usb_write(linc_usb_endpoint_t endpoint, const void* buffer, ULONG length)
{
    if (length > LINC_USB_MAX_PACKET_SIZE)
    {
        return UX_ERROR;
    }

    linc_usb_packet_t packet;

    packet.endpoint = endpoint;
    packet.length = length;
    memcpy(packet.data, buffer, length);

    return tx_queue_send(&usb.tx_queue, &packet, TX_NO_WAIT);
}

UX_SLAVE_CLASS_CDC_ACM* linc_usb_cdc(void) { return usb.cdc; }

VOID linc_usb_tx_thread_entry(ULONG thread_input)
{
    UX_PARAMETER_NOT_USED(thread_input);
    linc_usb_packet_t packet;
    while (1)
    {
        tx_queue_receive(&usb.tx_queue, &packet, TX_WAIT_FOREVER);
        if (!usb.connected)
        {
            continue;
        }
        switch (packet.endpoint)
        {
        case LINC_USB_ENDPOINT_CONSOLE:
        {
            ULONG actual_length;
            ux_device_class_cdc_acm_write(usb.cdc, packet.data, packet.length, &actual_length);
            break;
        }
        case LINC_USB_ENDPOINT_VENDOR:
        {
            /* TODO: Vendor endpoint implementation. */
            break;
        }
        default:
        {
            break;
        }
        }
    }
}

VOID linc_usb_rx_thread_entry(ULONG thread_input)
{
    UX_PARAMETER_NOT_USED(thread_input);
    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}