/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-04-28     zengjing     the first version
 */
#ifndef APPLICATIONS_BUS_SERVO_H_
#define APPLICATIONS_BUS_SERVO_H_

#include <rthw.h>
#include <rtthread.h>


struct bus_servo_device
{
    rt_device_t serial;
    rt_uint8_t id;
    rt_uint16_t value;
    rt_uint16_t time;
    rt_mutex_t lock;
};
typedef struct bus_servo_device *bus_servo_device_t;


bus_servo_device_t bus_servo_init(const char *serial_name);

void bus_servo_deinit(bus_servo_device_t dev);

void bus_servo_control(bus_servo_device_t dev);

#endif /* APPLICATIONS_BUS_SERVO_H_ */
