/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-4-18      zengjing     first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <aht10.h>
#include <buzzer.h>
#include <openmv.h>
#include <drv_lcd.h>
#include <rttlogo.h>
#include <robotic_arm.h>
#include <math.h>
#include <key_scan.h>
#include <led_matrix.h>
#include <wifi_onenet.h>

#define DBG_TAG "main"
#define DBG_LVL         DBG_LOG
#include <rtdbg.h>

struct rt_mailbox led_matrix_mb;
struct rt_mailbox buzzer_mb;

static char led_matrix_mb_pool[128];
static char buzzer_mb_pool[128];

double humidity = 0;
double temperature = 0;

static void lcd_humidity_entry()
{
    lcd_clear(WHITE1);
    lcd_set_color(WHITE1, BLACK);
    aht10_device_t use_aht10 = aht10_init("i2c3");
    while(1)
    {
        lcd_show_string(75, 200, 24, "Sensor");
        temperature =  aht10_read_temperature(use_aht10);
        humidity =  aht10_read_humidity(use_aht10);
        lcd_show_string(45, 69, 24, "temp: %d.%d",(int)temperature, (int)(temperature * 10) % 10);
        lcd_show_string(45, 69 + 48, 24, "humi: %d.%d %%", (int)humidity, (int)(humidity * 10) % 10);
        lcd_show_string(180-15+9, 69-6-2, 16, "o");
        lcd_show_string(180-15+8+8,69, 24,"C");
        lcd_show_image(15,69 + 48, 24, 24, gImage_water);
        lcd_show_image(18,65 , 15, 36, gImage_temp);
        rt_mb_send(&led_matrix_mb, (rt_uint32_t)temperature);
        rt_mb_send(&buzzer_mb, (rt_uint32_t)temperature);
        rt_thread_mdelay(500);
    }

}

int main(void)
{
    openmv_uart_init(RT_NULL);
    key_scan_init();
    wifi_init();
    rt_mb_init(&led_matrix_mb,"led_matrix_mbt",&led_matrix_mb_pool[0],sizeof(led_matrix_mb_pool) / 4,RT_IPC_FLAG_FIFO);
    rt_mb_init(&buzzer_mb,"buzzer_mbt",&buzzer_mb_pool[0],sizeof(buzzer_mb_pool) / 4,RT_IPC_FLAG_FIFO);

    rt_thread_t wifi_thread = rt_thread_create("wifi",wifi_entry,RT_NULL,1024,20,20);
    if (wifi_thread != RT_NULL)
        rt_thread_startup(wifi_thread);
    rt_thread_mdelay(200);

    rt_thread_t humidity_thread = rt_thread_create("humidity",lcd_humidity_entry, RT_NULL,1024,20, 20);
    if (humidity_thread != RT_NULL)
        rt_thread_startup(humidity_thread);

    rt_thread_t buzzer_thread = rt_thread_create("buzzer",buzzer_entry, RT_NULL,1024,20, 20);
    if (buzzer_thread != RT_NULL)
        rt_thread_startup(buzzer_thread);

    rt_thread_t openmv_thread = rt_thread_create("uart_openmv", openmv_uart_entry, RT_NULL, 1024, 20, 10);
    if (openmv_thread != RT_NULL)
        rt_thread_startup(openmv_thread);

    rt_thread_t led_matrix_thread = rt_thread_create("led_matrix", led_matrix_entry, RT_NULL, 1024, 20, 20);
    if (led_matrix_thread != RT_NULL)
        rt_thread_startup(led_matrix_thread);
    rt_thread_mdelay(200);

    rt_thread_t onenet_thread = rt_thread_create("onenet", onenet_entry, RT_NULL, 2048, 20, 20);
    if (onenet_thread != RT_NULL)
        rt_thread_startup(onenet_thread);
    rt_thread_mdelay(200);

    rt_thread_t robotic_arm_thread = rt_thread_create("robotic_arm", robotic_arm_entry, RT_NULL, 2048, 20, 20);
    if (robotic_arm_thread != RT_NULL)
        rt_thread_startup(robotic_arm_thread);
    rt_thread_mdelay(200);

    rt_thread_t key_scan_thread = rt_thread_create("key_scan",key_scan,RT_NULL,1024,20,1);
    if (key_scan_thread != RT_NULL)
        rt_thread_startup(key_scan_thread);
    rt_thread_mdelay(200);


    return 0;
}
