/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-04-28     admin       the first version
 */
#ifndef APPLICATIONS_OPENMV_H_
#define APPLICATIONS_OPENMV_H_
#include <rtthread.h>

extern float object_x_cm;
extern float object_y_cm;

extern rt_device_t openmv_serial;

extern uint16_t lable;

extern char Location[11];
extern char ID[21];
extern char global_status[11];

void openmv_uart_entry();

rt_err_t openmv_uart_init(void *parameter);

#endif /* APPLICATIONS_OPENMV_H_ */
