/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-19     zengjing     the first version
 */

#include <math.h>
#include <stdio.h>
#include <openmv.h>
#include <robotic_arm.h>
volatile rt_bool_t arm_ready = RT_TRUE;
target_angle_t angle;
servo_position_t position;

struct rt_semaphore openmv_sem;
bus_servo_device_t robotic_arm;

/*角度换算*/
static rt_uint16_t angle_to_position(float angle) {
    /*约束输入角度在有效范围内 [-90, 90]*/
    if(angle < -90.0) angle = -90.0;
    if(angle > 90.0) angle = 90.0;

    /*分段线性映射计算*/
    float position;
    if (angle <= 0.0) {
        /*-90° 到 0° 区间的映射*/
        position = 900.0 + (angle - (-90.0)) * (2000.0 - 900.0) / (0.0 - (-90.0));
    } else {
        /*0° 到 90° 区间的映射*/
        position = 2000.0 + (angle - 0.0) * (3072.0 - 2000.0) / (90.0 - 0.0);
    }

    return (rt_uint16_t)(position + 0.5);
}

/*转换每个角度到位置值*/
static void translate_angle_to_position(target_angle_t angle)
{
    position->servo_1 = angle_to_position(angle->angle_1);
    position->servo_2 = angle_to_position(angle->angle_2);
    position->servo_3 = angle_to_position(angle->angle_3);
    position->servo_4 = angle_to_position(angle->angle_4);
    position->servo_5 = angle_to_position(angle->angle_5);
    position->servo_6 = angle_to_position(angle->angle_6);

}

/*机械臂抓取*/
static void servo_catch(bus_servo_device_t dev, target_angle_t angle)
{
    translate_angle_to_position(angle);
    dev->time = 1000;
    if(position->servo_1 != 0.0)
    {
        dev->id = 1;
        dev->value = position->servo_1;
        bus_servo_control(dev);
    }
    rt_thread_mdelay(500);
    if(position->servo_6 != 0.0)
    {
        dev->id = 6;
        dev->value = 1300;
        bus_servo_control(dev);
    }
    rt_thread_mdelay(500);
    if(position->servo_4 != 0.0)
    {
        dev->id = 4;
        dev->value = position->servo_4;
        bus_servo_control(dev);
    }
    rt_thread_mdelay(500);
    if(position->servo_3 != 0.0)
    {
        dev->id = 3;
        dev->value = position->servo_3;
        bus_servo_control(dev);
    }
    rt_thread_mdelay(500);
    if(position->servo_2 != 0.0)
    {
        dev->id = 2;
        dev->value = position->servo_2;
        bus_servo_control(dev);
    }
    rt_thread_mdelay(500);
    if(position->servo_5 != 0.0)
    {
        dev->id = 5;
        dev->value = position->servo_5;
        bus_servo_control(dev);
    }
    rt_thread_mdelay(500);
    if(position->servo_6 != 0.0)
    {
        dev->id = 6;
        dev->value = 3072;
        bus_servo_control(dev);
    }
    rt_kprintf("catch yes\n");
    rt_thread_mdelay(500);
    rt_thread_mdelay(2000);
}

/*
函数功能：根据目标位置，计算目标角度
入口参数：末端执行器姿态
*/
static void servo_angle_calculate(float target_x, float target_y, float target_z)
{
    if (target_y >= 18)
        target_y = 18;
    else if(target_y <= 3)
        target_y = 3;
    float len_1, len_2, len_3, len_4;   //a1为底部圆台高度 剩下三个为三个机械臂长度
    float j1,j2,j3,j4 ;   //四个姿态角
    float L, H, bottom_r;   //  L = a2*sin(j2) + a3*sin(j2 + j3);H = a2*cos(j2) + a3*cos(j2 + j3); P为底部圆盘半径R
    float j_sum;            //j2,j3,j4之和
    float len, high;   //总长度,总高度
    float cos_j3, sin_j3; //用来存储cosj3,sinj3数值
    float cos_j2, sin_j2;
    float k1, k2;
    int i;
    float n, m;
    n = 0;
    m = 0;

    //输入初始值
    bottom_r = 5;      //底部圆盘半径
    len_1 = 8.5;     //底部圆盘高度
    //机械臂长度
    len_2 = 8.5;
    len_3 = 8.5;
    len_4 = 19;

    if (target_x == 0) {
        j1 = 0;  // 正前方为0度
    } else {
        // 计算角度（弧度）
        double angle_rad = - atan(target_x / (target_y + bottom_r));

        // 转换为角度并取负号（逆时针为负）
        j1 = angle_rad * 57.3;

        // 确保角度在-90°到90°之间
        if (j1 < -90) j1 = -90;
        if (j1 > 90) j1 = 90;
    }

    for (i = 0; i <= 180; i ++)
    {
        j_sum = 3.1415927 * i / 180;

        len = sqrt((target_y + bottom_r) * (target_y + bottom_r) + target_x * target_x);
        high = target_z;

        L = len - len_4 * sin(j_sum);
        H = high - len_4 * cos(j_sum) - len_1;

        cos_j3 = ((L * L) + (H * H) - ((len_2) * (len_2)) - ((len_3) * (len_3))) / (2 * (len_2) * (len_3));
        sin_j3 = (sqrt(1 - (cos_j3) * (cos_j3)));

        j3 = atan((sin_j3) / (cos_j3)) * (57.3);

        k2 = len_3 * sin(j3 / 57.3);
        k1 = len_2 + len_3 * cos(j3 / 57.3);

        cos_j2 = (k2 * L + k1 * H) / (k1 * k1 + k2 * k2);
        sin_j2 = (sqrt(1 - (cos_j2) * (cos_j2)));

        j2 = atan((sin_j2) / (cos_j2)) * 57.3;
        j4 = j_sum * 57.3 - j2 - j3;

        if (j2 >= 0 && j3 >= 0 && j4 >= -90 && j2 <= 180 && j3 <= 180 && j4 <= 90)
        {
            n ++;
        }
    }


    for (i = 0; i <= 180; i ++)
    {
        j_sum = 3.1415927 * i / 180;

        len = sqrt((target_y + bottom_r) * (target_y + bottom_r) + target_x * target_x);
        high = target_z;

        L = len - len_4 * sin(j_sum);
        H = high - len_4 * cos(j_sum) - len_1;

        cos_j3 = ((L * L) + (H * H) - ((len_2) * (len_2)) - ((len_3) * (len_3))) / (2 * (len_2) * (len_3));
        sin_j3 = (sqrt(1 - (cos_j3) * (cos_j3)));

        j3 = atan((sin_j3) / (cos_j3)) * (57.3);

        k2 = len_3 * sin(j3 / 57.3);
        k1 = len_2 + len_3 * cos(j3 / 57.3);

        cos_j2 = (k2 * L + k1 * H) / (k1 * k1 + k2 * k2);
        sin_j2 = (sqrt(1 - (cos_j2) * (cos_j2)));

        j2 = atan((sin_j2) / (cos_j2)) * 57.3;
        j4 = j_sum * 57.3 - j2 - j3;

        if (j2 >= 0 && j3 >= 0 && j4 >= -90 && j2 <= 180 && j3 <= 180 && j4 <= 90)
        {
            m ++;
            if (m == n / 2 || m == (n + 1) / 2)
                break;
        }
    }

    angle->angle_1 = j1;
    angle->angle_2 = j2;
    angle->angle_3 = j3;
    angle->angle_4 = j4;
    angle->angle_5 = -41;
    angle->angle_6 = 0;
    rt_kprintf("x: %f y: %f angle_1: %f angle_2: %f angle_3: %f angle_4: %f\r\n", target_x, target_y, j1, j2, j3, j4);
}

void servo_lift(bus_servo_device_t dev)
{
    dev->id = 2;
    dev->value = 2000;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);
    dev->id = 3;
    dev->value = 2357;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);
    dev->id = 4;
    dev->value = 2357;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);
    rt_kprintf("lift yes\n");
    rt_thread_mdelay(2000);

}

void servo_transfer_blue(bus_servo_device_t dev)
{
    dev->id = 1;
    dev->value = 1024;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);
    dev->id = 2;
    dev->value = 2500;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);
    dev->id = 6;
    dev->value = 1024;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);
    rt_thread_mdelay(2000);
}

void servo_transfer_red(bus_servo_device_t dev)
{
    dev->id = 1;
    dev->value = 3072;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);
    dev->id = 2;
    dev->value = 2500;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);
    dev->id = 6;
    dev->value = 1024;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);
    rt_thread_mdelay(2000);
    rt_kprintf("transfer yes\n");
}

void robotic_arm_reset(bus_servo_device_t dev)
{
    dev->time = 1000;
    dev->id = 0xfe;
    dev->value = 2048;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);

    dev->id = 5;
    dev->value = 1500;
    bus_servo_control(dev);
    rt_thread_mdelay(1000);
    rt_kprintf("robotic_arm_reset yes\n");
}

void robotic_arm_entry()
{
    angle = rt_calloc(1, sizeof(struct target_angle));
    position = rt_calloc(1, sizeof(struct servo_position));

    robotic_arm = bus_servo_init("uart3");
    rt_sem_init(&openmv_sem, "openmv_sem", 0, RT_IPC_FLAG_FIFO);

    robotic_arm_reset(robotic_arm);
    while (1)
    {

        rt_kprintf("arm_object_x_cm: %.2f  arm_object_y_cm: %.2f \r\n", object_x_cm, object_y_cm);
        rt_sem_take(&openmv_sem, RT_WAITING_FOREVER);

        /* 检查是否为有效指令 */
        if(object_x_cm == 0 && object_y_cm == 0) {
            continue; // 跳过无效指令
        }
        /* 设置工作状态 */
        arm_ready = RT_FALSE;

        uint16_t class = lable;;
        servo_angle_calculate(object_x_cm , object_y_cm , 1);

        servo_catch(robotic_arm, angle);
        servo_lift(robotic_arm);
        if(class == 0)
        {
            servo_transfer_blue(robotic_arm);
        }
        else if(class == 1)
        {
            servo_transfer_red(robotic_arm);
        }
        robotic_arm_reset(robotic_arm);
        rt_thread_mdelay(3000);
        object_x_cm = 0;
        object_y_cm = 0;
        char dummy;
        while (rt_device_read(openmv_serial, 0, &dummy, 1) > 0);
        arm_ready = RT_TRUE;
    }
}
