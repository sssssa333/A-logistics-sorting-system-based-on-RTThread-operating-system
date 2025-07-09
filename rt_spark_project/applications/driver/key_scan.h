/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-07-01     zengjing     the first version
 */
#ifndef APPLICATIONS_TASK_KEY_SCAN_H_
#define APPLICATIONS_TASK_KEY_SCAN_H_

#include <rtthread.h>

/* 配置 KEY 输入引脚  */
#define PIN_LEFT        GET_PIN(C, 0)
#define PIN_DOWN        GET_PIN(C, 1)
#define PIN_RIGHT       GET_PIN(C, 4)
#define PIN_UP          GET_PIN(C, 5)

#define SERVO_MIN 1024    // 逆时针极限位置
#define SERVO_MAX 3072    // 顺时针极限位置
#define SERVO_STEP 100     // 每次按键变化的步长

void key_scan();
void key_scan_init();

#endif /* APPLICATIONS_TASK_KEY_SCAN_H_ */
