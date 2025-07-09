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

struct servo_position arm_pos = {
    .servo_1 = 2000,
    .servo_2 = 2000,
    .servo_3 = 2000,
    .servo_4 = 2000,
    .servo_5 = 2000,
    .servo_6 = 2000
};

void move_servos_multi(const int *ids, int count, int delta, struct servo_position *pos)
{
    for (int i = 0; i < count; i++)
    {
        int id = ids[i];
        rt_uint16_t *target_servo = NULL;

        switch (id)
        {
            case 1: target_servo = &pos->servo_1; break;
            case 2: target_servo = &pos->servo_2; break;
            case 3: target_servo = &pos->servo_3; break;
            case 4: target_servo = &pos->servo_4; break;
            case 5: target_servo = &pos->servo_5; break;
            case 6: target_servo = &pos->servo_6; break;
        }

        if (target_servo == NULL) continue;

        int new_value = *target_servo + delta;
        if (new_value < SERVO_MIN) new_value = SERVO_MIN;
        if (new_value > SERVO_MAX) new_value = SERVO_MAX;

        *target_servo = new_value;

        robotic_arm->id = id;
        robotic_arm->value = new_value;
        bus_servo_control(robotic_arm);

//        rt_kprintf("Servo %d -> %d\n", id, new_value);
    }
}

void key_scan()
{
    while(1)
    {
        rt_thread_mdelay(15);
        if (rt_pin_read(PIN_RIGHT) == PIN_LOW)
        {
            int id1[] = {1};
            move_servos_multi(id1, 1, -SERVO_STEP, &arm_pos);
            rt_thread_mdelay(200);
        }

        if (rt_pin_read(PIN_LEFT) == PIN_LOW)
        {
            int id1[] = {1};
            move_servos_multi(id1, 1, SERVO_STEP, &arm_pos);
            rt_thread_mdelay(200);
        }
        if (rt_pin_read(PIN_UP) == PIN_LOW)
        {
            int ids[] = {2, 3, 4};
            move_servos_multi(ids, 3, SERVO_STEP, &arm_pos);
            rt_thread_mdelay(200);
        }

        if (rt_pin_read(PIN_DOWN) == PIN_LOW)
        {
            int ids[] = {2, 3, 4};
            move_servos_multi(ids, 3, -SERVO_STEP, &arm_pos);
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

