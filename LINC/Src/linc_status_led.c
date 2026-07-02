#include "linc_status_led.h"

#include "linc_usb_console.h"
#include "main.h"
#include "tx_api.h"

extern TX_SEMAPHORE semaphore;

VOID linc_status_led_thread_entry(ULONG thread_input)
{
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        linc_usb_console_write_string("Hello World123\r\n");
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2);
    }
}