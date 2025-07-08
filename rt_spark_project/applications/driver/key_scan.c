/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-07-01     zengjing     the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <key_scan.h>
#include <robotic_arm.h>

void key_scan()
{
    while(1)
    {
        rt_thread_mdelay(15);

        if (rt_pin_read(PIN_RIGHT) == PIN_LOW)
        {
            robotic_arm->id = 1;
            if (robotic_arm->value > SERVO_MIN)
            {
                robotic_arm->value -= SERVO_STEP;
                // 确保不低于最小值
                if (robotic_arm->value < SERVO_MIN) robotic_arm->value = SERVO_MIN;
                bus_servo_control(robotic_arm);
                rt_kprintf("right\n");
            }
            rt_thread_mdelay(200);
        }
        if (rt_pin_read(PIN_DOWN) == PIN_LOW)
        {

        }
        if (rt_pin_read(PIN_UP) == PIN_LOW)
        {

        }
        if (rt_pin_read(PIN_LEFT) == PIN_LOW)
        {
            robotic_arm->id = 1;
            if (robotic_arm->value < SERVO_MAX)
            {
                robotic_arm->value += SERVO_STEP;
                // 确保不高于最大值
                if (robotic_arm->value > SERVO_MAX) robotic_arm->value = SERVO_MAX;
                bus_servo_control(robotic_arm);
                rt_kprintf("left\n");
            }
            rt_thread_mdelay(200);
        }
    }
}

void key_scan_init()
{
    rt_pin_mode(PIN_LEFT, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(PIN_DOWN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(PIN_RIGHT, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(PIN_UP, PIN_MODE_INPUT_PULLUP);
}

