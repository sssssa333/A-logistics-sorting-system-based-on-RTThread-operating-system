/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-04-20     zengjing       the first version
 */
#include <stdio.h>
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <openmv.h>

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
            if (temperature >= 35 || temperature < 21)
            {
               buzzer_on();
            }
            else
            {
               buzzer_off();
            }
            if (strcmp(global_status, "damaged") == 0)
            {
               buzzer_on();
               rt_thread_mdelay(1000);
               buzzer_off();
               rt_thread_mdelay(1000);
            }
       }
}
