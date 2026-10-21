/**
  ******************************************************************************
  * @file       net.c
  * @author     应用层代码
  * @modifier   Yang Chenghao (杨承澔) 
  参考了野火官方的应用层代码，并加上了oled显示模式切换指令和加上了光照强度数据的传输
  ******************************************************************************
  */

#include "esp8266.h"
#include "oled.h"
#include "usart.h"
#include "display.h"
#include "dma.h"
#include <stdio.h>
#include <string.h>
#include "net.h"
#include <stdbool.h>
#include "dht11.h"
#include "light.h"
#include "cunchu.h"
extern volatile uint8_t ESP_RX_Complete;
extern uint8_t ESP8266_RxBuff[UART_RX_BUFF_LEN];
extern uint8_t stat;

// 你之前问过的全局变量（顺便确认下）
extern DHT11_DATA_TYPEDEF dht11_data;
extern float light;

void ESP8266_Task(void)
{
    // 打印初始化提示信息
    printf(">>> ESP8266 上电初始化...\r\n");

    ESP8266_Init();                                    // 初始化 ESP8266 模块及相关外设
    printf(">>> ESP8266 已初始化并开启接收\r\n");

    // 软件复位 ESP8266，并等待 ready
    printf(">>> ESP8266 软件复位中...\r\n");
    if(ESP8266_SoftReset_WaitReady())       // 软件复位并等待 ready
        printf(">>> ESP8266 复位完成，ready 已收到\r\n");
    else
    {
        printf(">>> ESP8266 复位超时!\r\n");
        return;                              // 复位失败则退出任务
    }
	
    // 测试 ESP8266 模块是否可用
    if(ESP8266_Test())
        printf(">>> ESP8266 模块测试通过\r\n");
    else
    {
        printf(">>> ESP8266 模块不可用!\r\n");
        return;                                       // 模块不可用则退出任务
    }

    // 设置工作模式为 STA
    if(ESP8266_SetMode(STA))
        printf(">>> ESP8266 已设置为 STA 模式\r\n");
    else
    {
        printf(">>> 设置工作模式失败!\r\n");
        return;                                       // 设置失败则退出任务
    }

    // 设置单连接模式
    if(ESP8266_SetConnMode(SINGLE_CONN))
        printf(">>> ESP8266 单连接模式已设置\r\n");
    else
    {
        printf(">>> 设置连接模式失败!\r\n");
        return;                                       // 设置失败则退出任务
    }

    // 连接指定 Wi-Fi
    if(!ESP8266_ConnectWiFi(macUser_ESP8266_ApSsid, macUser_ESP8266_ApPwd))
        printf(">>> Wi-Fi 已连接\r\n");
    else
		{
        printf(">>> Wi-Fi 连接失败! \r\n");
				return; 
		}
			
    // 连接 TCP 服务器
    if(ESP8266_ConnectTCPServer(macUser_ESP8266_TcpServer_IP, macUser_ESP8266_TcpServer_Port, 0))
        printf(">>> 已连接到 TCP 服务器\r\n");
    else
    {
        printf(">>> 连接 TCP 服务器失败!\r\n");
        return;                                       // 连接失败则退出任务
    }
		
    // 设置透传模式
    if(ESP8266_SetTransparentMode(TRANSPARENT_ON))
        printf(">>> ESP8266 已开启透传模式\r\n");
    else
		{
        printf(">>> 设置透传模式失败!\r\n");
				return; 
		}

    // 设置 ESP8266 进入发送数据模式
    if(ESP8266_SetSendMode())
        printf(">>> ESP8266 已进入发送数据模式\r\n>");
    else
		{
        printf(">>> 设置发送数据模式失败!\r\n");
				return; 
		}
}

/**
  * @brief  ESP8266 透传模式任务
  * @note   解析 TCP/UDP 接收到的命令并执行对应动作
  */



void ESP8266_SendSensorData_Task(void)
{

    char send_buf[128];
    
    if (DHT11_Get(&dht11_data) != HAL_OK) {

    return;
} else {
    printf(">>> 探针：WIFI模块运行正常 T:%d.%d H:%d.%d\r\n", 
           dht11_data.temp_int, dht11_data.temp_deci, 
           dht11_data.humi_int, dht11_data.humi_deci);
}

    // 1. 拼接并发送网络数据
    snprintf(send_buf, sizeof(send_buf), 
             "温度:%d.%d,湿度:%d.%d,光照:%.2f",
             dht11_data.temp_int, dht11_data.temp_deci, 
             dht11_data.humi_int, dht11_data.humi_deci, 
             light);
    ESP8266_SendString(send_buf);

    // 2. 顺便把数据存进 Flash 黑匣子
    // 注意：你需要在 main.c 或 net.c 顶部 extern 这两个变量
    float temp_float = dht11_data.temp_int + (dht11_data.temp_deci / 10.0f);
    float humi_float = dht11_data.humi_int + (dht11_data.humi_deci / 10.0f);
    
    Flash_SaveRecord(temp_float, humi_float, light);
		printf(">>> 网络上报数据：T=%d.%d, H=%d.%d\r\n", 
       dht11_data.temp_int, dht11_data.temp_deci,
       dht11_data.humi_int, dht11_data.humi_deci);
}

void ESP8266_TransparentTask(void)
{
    if (ESP_RX_Complete == 1) // 收到一帧数据
    {
        ESP_RX_Complete = 0;
        // 判断是不是切屏指令
        if (strstr((char *)ESP8266_RxBuff, "wen") != NULL) {
            stat = 0;
        }
        else if (strstr((char *)ESP8266_RxBuff, "guang") != NULL) {
            stat = 1;
        }
        memset(ESP8266_RxBuff, 0, UART_RX_BUFF_LEN); // 清空缓冲
    }
}