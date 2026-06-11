#include "linc_status_led.h"
#include "main.h"

void linc_status_led_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2);
    }
}