from pyb import UART
import sensor
import time
import math
import sensor, image, time, os, tf, uos, gc,math, struct, lcd
thresholds = [
    (88, 21, -128, 87, -128, -26),  # 蓝色
    (0, 100, 19, 127, -25, 127),    # 红色
    # (31, 100, -57, -18, 13, 127)
    (100, 100, 99, -18, 13, 127),    # 绿色
]

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.VGA)
# sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time=2000)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)
# sensor.set_windowing((0, 0, 320, 240))
sensor.set_windowing((0, 0, 640, 480))
clock = time.clock()

# 定义颜色名称和对应的绘制颜色
colors = [
    {"name": "Blue", "draw_color": (0, 0, 255)},   # 蓝色
    {"name": "Red", "draw_color": (255, 0, 0)},     # 红色
    {"name": "Green", "draw_color": (0, 255, 0)},   # 绿色
]

uart = UART(2, 0)  # 初始化串口 波特率 115200

def UartSendData(Data):
    uart.write(Data)

def QRDataPack(content):
    # 将内容转换为字节数组
    content_bytes = content.encode('utf-8')
    content_len = len(content_bytes)

    # 构建数据包基础结构
    pack_data = bytearray([
        0xAA,  # 帧头1
        0x30,  # 帧头2 (二维码标识)
        0x43,  # ID (二维码类型)
        0x00,  # 数据长度占位 (将在后面更新)
    ])

    # 添加内容长度 (2字节)
    pack_data.append(content_len >> 8)
    pack_data.append(content_len & 0xFF)

    # 添加二维码内容
    pack_data.extend(content_bytes)

    # 添加校验和占位
    pack_data.append(0x00)

    # 更新数据长度字段 (整个数据部分长度)
    data_length = len(pack_data) - 4  # 减去帧头(2)+ID(1)+长度字段(1)
    pack_data[3] = data_length

    # 计算校验和 (从帧头开始到校验和前)
    checksum = sum(pack_data[:-1]) & 0xFF
    pack_data[-1] = checksum

    return pack_data

def ObjectDataPackColor(flag, x, y):
    pack_data = bytearray(
            [
                0xAA,
                0x29,
                0x42,
                0x00,
                flag,
                x >> 8,
                x,
                y>> 8,
                y,
                0x00,
            ]
        )
    lens = len(pack_data)  # 数据包大小
    pack_data[3] = 5
    # 有效数据个数
    i = 0
    sum = 0
    # 和校验
    while i < (lens - 1):
        sum = sum + pack_data[i]
        i = i + 1
    pack_data[lens - 1] = sum
    return pack_data

last_qr_content = None
qr_send_delay = 2.0  # 相同二维码发送间隔(秒)
last_qr_send_time = time.time()

def send_qr_code(content):
    """通过串口发送二维码内容"""
    if len(content) > 40:
        content = content[:40]  # 截断过长的内容

    packet = QRDataPack(content)
    # packet = QRDataPack("TEST")
    uart.write(packet)
    print(f"[UART Sent] QR: {content}")

# 物体状态帧发送函数（字符串版本）
def send_object_status(status_str):
   # 将字符串转换为字节数组
   status_bytes = status_str.encode('utf-8')
   str_len = len(status_bytes)

   # 检查长度限制
   if str_len > 40:
       status_bytes = status_bytes[:40]
       str_len = 40

   frame = bytearray()
   frame.append(0xAA)       # 帧头1
   frame.append(0x31)       # 帧头2（字符串状态）
   frame.append(0x44)       # 帧ID
   frame.append(str_len)    # 字符串长度

   # 添加字符串数据
   frame.extend(status_bytes)

   # 计算校验和（除校验和外所有字节和）
   checksum = sum(frame) & 0xFF
   frame.append(checksum)

   uart.write(frame)
   print(f"Sent status: {status_str}")

last_status = None
last_status_send_time = 0
status_send_delay = 0.5  # 状态发送间隔（秒）

net = None
labels = None

class object(object):
    flag = 0
    fps = 0
object = object()

try:
    # load the model, alloc the model file on the heap if we have at least 64K free after loading
    net = tf.load("trained.tflite", load_to_fb=uos.stat('trained.tflite')[6] > (gc.mem_free() - (64*1024)))
except Exception as e:
    print(e)
    raise Exception('Failed to load "trained.tflite", did you copy the .tflite and labels.txt file onto the mass-storage device? (' + str(e) + ')')

try:
    labels = [line.rstrip('\n') for line in open("labels.txt")]
except Exception as e:
    raise Exception('Failed to load "labels.txt", did you copy the .tflite and labels.txt file onto the mass-storage device? (' + str(e) + ')')

while True:
    clock.tick()
    current_time = time.time()
    img = sensor.snapshot()
    status_detected = False
    # 二维码检测与发送
    qr_detected = False
    for code in img.find_qrcodes():
        content = code.payload()
        print(f"QR detected: {content}")

        # 避免重复发送相同二维码
        if (content != last_qr_content or
            (current_time - last_qr_send_time) > qr_send_delay):
            send_qr_code(content)
            last_qr_content = content
            last_qr_send_time = current_time

        qr_detected = True
        # 可选: 在图像上绘制二维码区域
        img.draw_rectangle(code.rect(),color=(0, 0, 0))
        # img.draw_string(code.x(), code.y() - 10, content, color=(255, 0, 0))

    for obj in net.classify(img, min_scale=1.0, scale_mul=0.8, x_overlap=0.5, y_overlap=0.5):
        predictions_list = list(zip(labels, obj.output()))
        current_status = None  # 初始化为 None

        # 只有满足置信度阈值时才设置状态
        if predictions_list[0][1] > 0.7:
            current_status = predictions_list[0][0]
            print("Status:", current_status)
            print("damaged:", predictions_list[0][1])
            print("intact:", predictions_list[1][1])
        elif predictions_list[1][1] > 0.7:
            current_status = predictions_list[1][0]
            print("Status:", current_status)
            print("damaged:", predictions_list[0][1])
            print("intact:", predictions_list[1][1])
        # 如果都不满足条件，current_status 保持为 None

        # 只有检测到有效状态时才发送
        if current_status is not None:
            if (current_status != last_status or
                (current_time - last_status_send_time) > status_send_delay):
                send_object_status(current_status)
                last_status = current_status
                last_status_send_time = current_time
                status_detected = True

    # 颜色块检测与发送
    detected_colors = []
    for color_index in range(len(thresholds)):
        blobs = img.find_blobs(
            [thresholds[color_index]],
            pixels_threshold=200,
            area_threshold=200,
            merge=True,
        )

        for blob in blobs:
            x = blob.cx()
            y = blob.cy()
            detected_colors.append({
                "name": colors[color_index]["name"],
                "x": x,
                "y": y
            })

            # 绘制和发送颜色数据 (保持你原有的逻辑)
            draw_color = colors[color_index]["draw_color"]
            img.draw_rectangle(blob.rect(), color=draw_color)
            img.draw_cross(x, y, color=draw_color)
            img.draw_string(x + 10, y + 10, colors[color_index]["name"], color=draw_color)

            if blob.elongation() > 0.5:
                img.draw_edges(blob.min_corners(), color=draw_color)
                img.draw_line(blob.major_axis_line(), color=draw_color)
                img.draw_line(blob.minor_axis_line(), color=draw_color)

            img.draw_keypoints([(x, y, int(math.degrees(blob.rotation())))], size=20, color=draw_color)
            UartSendData(
                ObjectDataPackColor(
                    int(color_index),  # 颜色索引 (0,1,2)
                    x,                 # X坐标
                    y                  # Y坐标
                )
            )

    # 调试信息输出
    # if not detected_colors and not qr_detected:
    #     print("No objects detected")
    # else:
    #     for color_info in detected_colors:
    #         print(f"{color_info['name']}: x={color_info['x']}, y={color_info['y']}")

    # print("FPS:", clock.fps())
    if not detected_colors and not qr_detected and not status_detected:
        print("No objects detected")
    else:
      # 如果检测到状态但未检测到颜色块，也打印状态信息
        if status_detected:
            print(f"Status: {last_status}")

        for color_info in detected_colors:
            print(f"{color_info['name']}: x={color_info['x']}, y={color_info['y']}")

    print("FPS:", clock.fps())
