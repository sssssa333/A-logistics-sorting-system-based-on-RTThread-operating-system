/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-04-20     zengjing       the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

extern struct rt_mailbox buzzer_mb;

#define PIN_BUZZER  GET_PIN(B, 0)

void buzzer_on(void)
{
    rt_pin_mode(PIN_BUZZER, PIN_MODE_OUTPUT);
    rt_pin_write(PIN_BUZZER, 1);
}

void buzzer_off(void)
{
    rt_pin_mode(PIN_BUZZER, PIN_MODE_OUTPUT);
    rt_pin_write(PIN_BUZZER, 0);
}

void buzzer_entry()
{
    int temperature;

    while (1)
       {
            rt_mb_recv(&buzzer_mb, (rt_uint32_t *)&temperature, RT_WAITING_FOREVER);
            if (temperature >= 31 || temperature < 21)
            {
//               buzzer_on();
            }
            else
            {
//               buzzer_off();
            }
       }
}
