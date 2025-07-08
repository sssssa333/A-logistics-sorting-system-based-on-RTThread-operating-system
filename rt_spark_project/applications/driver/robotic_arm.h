/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-19     zengjing     the first version
 */
#ifndef APPLICATIONS_TASK_ROBOTIC_ARM_H_
#define APPLICATIONS_TASK_ROBOTIC_ARM_H_
#include <bus_servo.h>

extern bus_servo_device_t robotic_arm;

struct target_angle
{
    float angle_1;
    float angle_2;
    float angle_3;
    float angle_4;
    float angle_5;
    float angle_6;
};
typedef struct target_angle *target_angle_t;

struct servo_position
{
    rt_uint16_t servo_1;
    rt_uint16_t servo_2;
    rt_uint16_t servo_3;
    rt_uint16_t servo_4;
    rt_uint16_t servo_5;
    rt_uint16_t servo_6;
};
typedef struct servo_position *servo_position_t;

void robotic_arm_entry();
void robotic_arm_reset(bus_servo_device_t dev);
void servo_lift(bus_servo_device_t dev);

#endif /* APPLICATIONS_TASK_ROBOTIC_ARM_H_ */
