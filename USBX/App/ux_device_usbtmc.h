//
// Minimal USBX USBTMC device class skeleton.
// Based on the USBX DPUMP class pattern: one Bulk OUT endpoint and one Bulk IN endpoint.
//

#ifndef UX_DEVICE_USBTMC_H
#define UX_DEVICE_USBTMC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ux_api.h"
#include "ux_device_stack.h"

#define UX_DEVICE_USBTMC_CLASS_NAME ((UCHAR *)"ux_device_usbtmc")

#define USBTMC_REQUEST_GET_CAPABILITIES  7U

#define USBTMC_STATUS_SUCCESS            0x01U

#define USBTMC_BCD_VERSION               0x0100U

typedef struct UX_DEVICE_USBTMC_STRUCT
{
    UX_SLAVE_INTERFACE *interface;
    UX_SLAVE_ENDPOINT  *bulk_in_endpoint;
    UX_SLAVE_ENDPOINT  *bulk_out_endpoint;
} UX_DEVICE_USBTMC;

/* USBX class entry point. */
UINT ux_device_usbtmc_entry(UX_SLAVE_CLASS_COMMAND *command);

/* Blocking Bulk OUT read. */
UINT ux_device_usbtmc_read(UX_DEVICE_USBTMC *usbtmc,
                           UCHAR *buffer,
                           ULONG requested_length,
                           ULONG *actual_length);

/* Blocking Bulk IN write. */
UINT ux_device_usbtmc_write(UX_DEVICE_USBTMC *usbtmc,
                            const UCHAR *buffer,
                            ULONG requested_length,
                            ULONG *actual_length);

#ifdef __cplusplus
}
#endif

#endif /* UX_DEVICE_USBTMC_H */
