#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include <string.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "VISION_OPENMV"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "openmv.h"
#include "cJSON.h"
#include "cJSON_Utils.h"

#define UART_NAME    "uart2"

static struct rt_semaphore rx_sem;

rt_device_t openmv_serial;

extern volatile rt_bool_t arm_ready;

float object_x_cm;
float object_y_cm;

extern struct rt_semaphore openmv_sem;

uint16_t lable;

char Location[11];
char ID[21];
char global_status[11];

void coordinate_transformation(int16_t object_x, int16_t object_y)
{
//    double ratio_x = 29.7 / 192.0;
//    double ratio_y = 21.0 / 156.0;
//    object_x_cm = (object_x - 160) * ratio_x;
//    object_y_cm = (240 - object_y) * ratio_y;

    double ratio_x = 29.7 / 385.0;
    double ratio_y = 21.0 / 291.0;
    object_x_cm = (object_x - 320) * ratio_x;
    object_y_cm = (480 - object_y) * ratio_y;
}


static void uart_parse_frame(uint8_t *buffer, uint8_t length)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < (length - 1); i++)
        sum += *(buffer + i);
    if (!(sum == *(buffer + length - 1)))
        return;

    /* 色块位置 */
    if (buffer[0] == 0xAA && buffer[1] == 0x29 && buffer[2] == 0x42)
    {
        lable = buffer[4];
        int16_t object_x = ((uint16_t)buffer[5] << 8) + buffer[6];
        int16_t object_y = ((uint16_t)buffer[7] << 8) + buffer[8];
        uint16_t fps = buffer[9];
        coordinate_transformation(object_x,object_y);
        rt_kprintf("lable:%d object_x:%d object_y:%d object_x_cm:%.2f object_y_cm:%.2f fps:%d\n", lable, object_x, object_y, object_x_cm, object_y_cm, fps);
    }
    /* 二维码信息 */
    else if (buffer[0] == 0xAA && buffer[1] == 0x30 && buffer[2] == 0x43)
    {
        // 从位置4和5获取内容长度
        uint16_t content_len = ((uint16_t)buffer[4] << 8) | buffer[5];

        // 防止溢出
        if (content_len > 40) content_len = 40;

        // 提取内容（从位置6开始）
        char qr_content[41] = {0};
        for (int i = 0; i < content_len; i++)
        {
            qr_content[i] = buffer[6 + i];
        }

        cJSON *json = cJSON_Parse(qr_content);
        if (json)
        {
            cJSON *location = cJSON_GetObjectItemCaseSensitive(json, "loc");
            cJSON *order_id = cJSON_GetObjectItemCaseSensitive(json, "id");

            if (cJSON_IsString(location) && cJSON_IsString(order_id))
            {
                strncpy(Location, location->valuestring, sizeof(Location) - 1);
                strncpy(ID, order_id->valuestring, sizeof(ID) - 1);
                rt_kprintf("地点: %s\n订单号: %s\n",Location,ID);
            } else
            {
                rt_kprintf("Invalid JSON format\n");
            }
            cJSON_Delete(json);
        } else
        {
            rt_kprintf("JSON parse error\n");
        }
    }
    else if (buffer[0] == 0xAA && buffer[1] == 0x31 && buffer[2] == 0x44)
    {
        // 验证长度字段
        uint8_t str_len = buffer[3];
        if (str_len == 0 || (4 + str_len + 1) != length) {
            rt_kprintf("Invalid status frame length\n");
            return;
        }

        // 提取字符串并添加终止符
        char status_str[20] = {0};
        memcpy(status_str, &buffer[4], str_len);
        status_str[str_len] = '\0';  // 确保字符串终止

        rt_kprintf("Object Status: %s\n", status_str);
        strncpy(global_status, status_str, sizeof(global_status)-1);
    }
}


static int uart_receive_byte(uint8_t data)
{
    static uint8_t RxBuffer[50];
    static uint8_t _data_cnt = 0;
    static uint16_t _data_len = 0;
    static uint8_t state = 0;

    /* 帧头1 */
    if (state == 0 && data == 0xAA) {
        _data_cnt = 0;
        state = 1;
        RxBuffer[0] = data;
    }
    /* 帧头2 - 区分数据类型 */
    else if (state == 1) {
        if (data == 0x29) {  // 色块数据
            _data_cnt = 0;  // 重置计数器
            state = 2;
            RxBuffer[1] = data;
        }
        else if (data == 0x30) {  // 二维码数据
            _data_cnt = 0;  // 重置计数器 - 关键修复！
            state = 20;
            RxBuffer[1] = data;
        }
        else if (data == 0x31) {  // 字符串状态数据
            _data_cnt = 0;
            state = 30;
            RxBuffer[1] = data;
        }
        else {
            state = 0;  // 无效帧头
        }
    }
    /* 色块数据: ID */
    else if (state == 2 && data < 0xF1)
    {
        state = 3;
        RxBuffer[2] = data;
    }
    /* 色块数据: 数据长度 */
    else if (state == 3 && data < 50)
    {
        state = 4;
        RxBuffer[3] = data;
        _data_len = data;
    }
    /* 色块数据: 数据 */
    else if (state == 4)
    {
        RxBuffer[4 + _data_cnt++] = data;
        if (_data_cnt >= _data_len)
            state = 5;
    }
    /* 色块数据: 校验和 */
    else if (state == 5)
    {
        state = 0;
        RxBuffer[4 + _data_cnt] = data; // 校验和位置

        uart_parse_frame(RxBuffer, 4 + _data_len + 1); // 完整帧长度
        return 1;
    }
    /* 二维码数据: ID (0x43) */
    else if (state == 20) {
        if (data == 0x43) {
            state = 21;
            RxBuffer[2] = data;
        } else {
            state = 0;  // 无效ID
        }
    }
    else if (state == 21) {
        state = 22;
        RxBuffer[3] = data;
    }
    else if (state == 22) {
        state = 23;
        RxBuffer[4] = data;
    }
    else if (state == 23) {
        state = 24;
        RxBuffer[5] = data;
        _data_len = ((uint16_t)RxBuffer[4] << 8) | RxBuffer[5];
        if (_data_len > 40) {
            state = 0;  // 长度超限，重置状态机
            return 0;
        }
        uint8_t expected_length = 2 + _data_len + 1;
        if (RxBuffer[3] != expected_length) {
            state = 0;  // 长度不匹配，重置状态机
            return 0;
        }
    }
    /* 二维码数据: 内容 */
    else if (state == 24) {
        if ((6 + _data_cnt) < sizeof(RxBuffer)) {
            RxBuffer[6 + _data_cnt] = data;
        }
        _data_cnt++;

        if (_data_cnt >= _data_len) {
            state = 25;  // 等待校验和
        }
    }
    /* 二维码数据: 校验和 */
    else if (state == 25) {
        state = 0;
        // 存储校验和 - 确保不越界
        if ((6 + _data_len) < sizeof(RxBuffer)) {
            RxBuffer[6 + _data_len] = data;
        }
        // 完整帧长度
        uint8_t total_len = 6 + _data_len + 1;
        uart_parse_frame(RxBuffer, total_len);
        return 1;
    }
    /* 字符串状态帧: ID (0x44) */
    else if (state == 30) {
        if (data == 0x44) {
            state = 31;
            RxBuffer[2] = data;
        } else {
            state = 0;
        }
    }
    /* 字符串状态帧: 数据长度 */
    else if (state == 31) {
        if (data > 0 && data <= 40) {  // 限制最大长度40字节
            state = 32;
            RxBuffer[3] = data;
            _data_len = data;
        } else {
            state = 0;  // 无效长度
        }
    }
    /* 字符串状态帧: 字符串数据 */
    else if (state == 32) {
        // 存储数据并检查是否完成
        RxBuffer[4 + _data_cnt] = data;
        _data_cnt++;

        if (_data_cnt >= _data_len) {
            state = 33;  // 等待校验和
        }
    }
    /* 字符串状态帧: 校验和 */
    else if (state == 33) {
        state = 0;
        // 存储校验和
        RxBuffer[4 + _data_len] = data;

        // 计算完整帧长度 = 帧头(3) + 长度(1) + 数据(N) + 校验和(1)
        uint8_t total_len = 5 + _data_len;
        uart_parse_frame(RxBuffer, total_len);
        return 1;
    }
    return 0;
}

/* 接收数据回调函数 */
static rt_err_t uart_rx_callback(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    if (size > 0)
    {
        rt_sem_release(&rx_sem);
    }
    return RT_EOK;
}


/* 数据解析线程 */
void openmv_uart_entry()
{
    char data;
    while (1)
    {
        while (rt_device_read(openmv_serial, 0, &data, 1) > 0);
        /* 从串口读取数据，没有读取到则等待接收信号量 */
        if (rt_device_read(openmv_serial, 0, &data, 1) == 0)
        {
            /* 清除信号量状态 */
            rt_sem_control(&rx_sem, RT_IPC_CMD_RESET, RT_NULL);

            /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        }
        /* 数据处理 */
        if (uart_receive_byte(data))
        {
            /* 只在机械臂就绪时发送信号 */
            if (arm_ready) {
                rt_sem_release(&openmv_sem);
            }
        }
    }
}

/* 串口初始化*/
rt_err_t openmv_uart_init(void *parameter)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 初始化配置参数 (旧版串口配置) */

    /* 查找系统中的串口设备 */
    openmv_serial = rt_device_find(UART_NAME);
    if (!openmv_serial)
    {
        rt_kprintf("find %s failed!\n", UART_NAME);
        return RT_ERROR;
    }

    /* 修改串口配置参数*/
    config.baud_rate = 115200;          //修改波特率为 115200
    config.data_bits = DATA_BITS_8;     //数据位 8
    config.stop_bits = STOP_BITS_1;     //停止位 1
    config.bufsz     = 512;             //修改缓冲区
    config.parity    = PARITY_NONE;     //无奇偶校验位

    /* 控制串口设备。通过控制接口传入命令控制字，与控制参数 */
    rt_device_control(openmv_serial, RT_DEVICE_CTRL_CONFIG, &config);

    /* 初始化信号量 */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);

    /* 以中断接收及轮询发送模式打开串口设备 */
    rt_device_open(openmv_serial, RT_DEVICE_FLAG_INT_RX);

    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(openmv_serial, uart_rx_callback);

    return RT_EOK;
}
