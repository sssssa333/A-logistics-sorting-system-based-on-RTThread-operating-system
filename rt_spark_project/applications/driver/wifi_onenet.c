/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-07-07     zengjing     the first version
 */

#include <rtthread.h>
#include <wlan_mgnt.h>
#include <wlan_prot.h>
#include <wlan_cfg.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <cJSON_util.h>
#include <paho_mqtt.h>
#include <onenet.h>

#include <openmv.h>
#include <led.h>
#include <drv_lcd.h>
#include <string.h>
#define DBG_TAG "wifi_onenet"
#define DBG_LVL         DBG_LOG
#include <rtdbg.h>

void onenet_callback(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size);

static rt_sem_t wifi_send;

extern double humidity;
extern double temperature;

#define WLAN_SSID      "12222"
#define WLAN_PASSWORD  "88888888"

void wifi_entry()
{

    while(1)
    {
        if(rt_wlan_is_connected() == RT_FALSE)
        {
            LOG_E("wifi disconnect");
            lcd_show_string(10, 10, 24, "WiFi: Disconnected");
            led_off_green();
            led_on_red();
        }else
        {
            rt_sem_release(wifi_send);
            lcd_show_string(10, 10, 24, "WiFi: Connected   ");
            led_on_green();
            led_off_red();
        }
        rt_thread_mdelay(5000);
    }
}

void onenet_entry()
{
    /* 下发命令回调函数设置 */
    onenet_set_cmd_rsp_cb(onenet_callback);

    double onenet_temp, onenet_humi;

    while(1)
    {
        rt_sem_take(wifi_send,RT_WAITING_FOREVER);

        if(rt_wlan_is_connected() == RT_FALSE)
        {
            continue;
        }
        onenet_temp = round(temperature * 100) / 100.0;
        onenet_humi = round(humidity * 100) / 100.0;
        /* 传感器数据上传 */
        if (onenet_mqtt_upload_digit("Temperature", onenet_temp) != RT_EOK) {
            rt_kprintf("Temperature upload failed!\n");
        }

        if (onenet_mqtt_upload_digit("Humidity", onenet_humi) != RT_EOK) {
            rt_kprintf("Humidity upload failed!\n");
        }
        onenet_mqtt_upload_string("id", ID);
        onenet_mqtt_upload_string("location", Location);
        onenet_mqtt_upload_string("state", global_status);

        rt_thread_mdelay(5000);
    }
}

/* 下发命令回调函数 */
void onenet_callback(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size)
{
    char res_buf[] = { "cmd is received!\n" };

    if (recv_size > 0)
    {
        char data = *recv_data;
        rt_kprintf("Command 1 received\n");
        /* 根据接收到的命令执行相应操作 */
        switch (data)
        {
        case '1':
            rt_kprintf("Command 1 received\n");
            break;
        case '2':
            rt_kprintf("Command 2 received\n");
            break;
        default:
            rt_kprintf("Unknown command received: %c\n", data);
            break;
        }
    }

    /* 回应 */
    *resp_data = (uint8_t *)ONENET_MALLOC(strlen(res_buf));
    strncpy((char *)*resp_data, res_buf, strlen(res_buf));
    *resp_size = strlen(res_buf);

}

int wifi_init(void)
{
    rt_err_t result = RT_EOK;
    wifi_send = rt_sem_create("wifi_sem", 0, RT_IPC_FLAG_FIFO);
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    led_off_green();
    led_on_red();
    /* 自动连接 */
    rt_wlan_config_autoreconnect(RT_TRUE);

    if(rt_wlan_scan() == RT_EOK)
    {
        LOG_D("the scan is started... ");
    }else
    {
        LOG_E("scan failed");
    }
    result = rt_wlan_connect(WLAN_SSID, WLAN_PASSWORD);
    if (result == RT_EOK)
    {
        LOG_D("the connect is ready... ");
        led_on_green();
        led_off_red();
        /* onenet初始化 */
        onenet_mqtt_init();
    }

    return 0;
}


