/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-04-28     zengjing       the first version
 */
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include <string.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "BUS_SERVO"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "bus_servo.h"


/* 控制总线舵机，
 * id：要控制的id号，0xfe为全体控制
 * value：位置值（96~4000）
 * time：运行的时间，时间越小，运行越快，最小为0
 * */
void bus_servo_control(bus_servo_device_t dev)
{
    if (dev->value >= 96 && dev->value <= 4000)
    {
        const rt_uint8_t s_id = dev->id & 0xff;
        const rt_uint8_t len = 0x07;
        const rt_uint8_t cmd = 0x03;
        const rt_uint8_t addr = 0x2a;

        const rt_uint8_t pos_H = (dev->value >> 8) & 0xff;
        const rt_uint8_t pos_L = dev->value & 0xff;

        const rt_uint8_t time_H = (dev->time >> 8) & 0xff;
        const rt_uint8_t time_L = dev->time & 0xff;

        const rt_uint8_t checknum = (~(s_id + len + cmd + addr + pos_H + pos_L + time_H + time_L)) & 0xff;
        // unsigned char data[] = {0xff, 0xff, s_id, len, cmd, addr, pos_H, pos_L, time_H, time_L, checknum};
        unsigned char data[11] = {0};
        data[0] = 0xff;
        data[1] = 0xff;
        data[2] = s_id;
        data[3] = len;
        data[4] = cmd;
        data[5] = addr;
        data[6] = pos_H;
        data[7] = pos_L;
        data[8] = time_H;
        data[9] = time_L;
        data[10] = checknum;

        if (rt_mutex_take(dev->lock, RT_WAITING_FOREVER) == RT_EOK)
        {
            /*一次性发送整个数据包 */
            rt_device_write(dev->serial, 0, data, sizeof(data));

            /*释放互斥锁*/
            rt_mutex_release(dev->lock);
        }
        else
        {
            rt_kprintf("Failed to acquire servo mutex!\n");
        }
    }
}


static void uart_init(bus_servo_device_t dev)
{

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    /* 修改串口配置参数*/
    config.baud_rate = 115200;          //修改波特率为 115200
    config.data_bits = DATA_BITS_8;     //数据位 8
    config.stop_bits = STOP_BITS_1;     //停止位 1
    config.bufsz     = 512;             //修改缓冲区
    config.parity    = PARITY_NONE;     //无奇偶校验位

    rt_device_control(dev->serial, RT_DEVICE_CTRL_CONFIG, &config);
    rt_device_open(dev->serial, RT_DEVICE_FLAG_INT_RX);

}

bus_servo_device_t bus_servo_init(const char *serial_name)
{
    bus_servo_device_t dev;

    RT_ASSERT(serial_name);

    dev = rt_calloc(1, sizeof(struct bus_servo_device));
    if (dev == RT_NULL)
    {
        LOG_E("Can't allocate memory for bus servo device on '%s' ", serial_name);
        return RT_NULL;
    }

    dev->serial = rt_device_find(serial_name);
    if (dev->serial == RT_NULL)
    {
        LOG_E("Can't find bus servo device on '%s' ", serial_name);
        rt_free(dev);
        return RT_NULL;
    }

    dev->lock = rt_mutex_create("mutex_bus_servo", RT_IPC_FLAG_FIFO);
    if (dev->lock == RT_NULL)
    {
        LOG_E("Can't create mutex for bus servo device on '%s' ", serial_name);
        rt_free(dev);
        return RT_NULL;
    }

    uart_init(dev);

    return dev;
}

void bus_servo_deinit(bus_servo_device_t dev)
{

    RT_ASSERT(dev);

    rt_mutex_delete(dev->lock);

    rt_free(dev);

}

