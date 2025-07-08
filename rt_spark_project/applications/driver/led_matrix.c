/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-05-02     zengjing     the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define DBG_TAG "led_matrix"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#include <led_matrix.h>
#include <drv_matrix_led.h>

extern struct rt_mailbox led_matrix_mb;

void led_matrix_entry()
{
    int temperature;

    while (1)
    {
        rt_mb_recv(&led_matrix_mb, (rt_uint32_t *)&temperature, RT_WAITING_FOREVER);
        if (temperature >= 31)
        {
            for (int i = EXTERN_LED_0; i <= EXTERN_LED_18; i++)
            {
                led_matrix_set_color(i, RED);
                rt_thread_mdelay(20);
                led_matrix_reflash();
            }
        }
        else if (temperature < 21)
        {
            for (int i = EXTERN_LED_0; i <= EXTERN_LED_18; i++)
            {
                led_matrix_set_color(i, BLUE);
                rt_thread_mdelay(20);
                led_matrix_reflash();
            }
        }
        else
        {
            for (int i = EXTERN_LED_0; i <= EXTERN_LED_18; i++)
            {
                led_matrix_set_color(i, GREEN);
                rt_thread_mdelay(20);
                led_matrix_reflash();
            }
        }
    }
}
