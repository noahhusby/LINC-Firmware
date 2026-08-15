//
// Created by Noah Husby on 8/15/26.
//

#include "main.h"

extern TIM_HandleTypeDef htim7;

void linc_time_init(void) { HAL_TIM_Base_Start(&htim7); }

void linc_time_delay_us(uint16_t us)
{
    uint16_t start = __HAL_TIM_GET_COUNTER(&htim7);

    while ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim7) - start) < us)
    {
    }
}