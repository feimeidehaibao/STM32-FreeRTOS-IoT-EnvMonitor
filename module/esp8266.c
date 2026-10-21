/**
  ******************************************************************************
  * @file       esp8266.c
  * @author     embedfire (底层驱动原始框架)
  * @modifier   Yang Chenghao (杨承澔) 
  * @brief      ESP8266 驱动与应用层封装
  * @note       原版基于野火开源教程。在此基础上，个人重构了部分逻辑：
  *             1. 增加了非阻塞式的透传接收回调
  *             2. 完善了 TCP 断线检测与重连机制 (利用字符串匹配)
  *             3. 适配了 FreeRTOS 的多任务调度
  ******************************************************************************
  */
  
#include "esp8266.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"
#include <stdbool.h>
#include "dma.h"
/* 分别定义两个接收缓冲区 */
uint8_t Debug_RxBuff[UART_RX_BUFF_LEN];      // 上位机串口接收缓冲
uint8_t ESP8266_RxBuff[UART_RX_BUFF_LEN];    // ESP8266串口接收缓冲

volatile bool ESP_RX_Complete = 0;	// ESP8266 接收完成标志（UART/ DMA 发送完成后置位）
volatile bool ESP_TX_Complete = 0;  // ESP8266 发送完成标志（UART 发送完成后置位）
volatile bool ESP8266_TransparentMode = 0;  // 0 = 关闭回显, 1 = 透传模式下开启回显


void ESP8266_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};    // GPIO 配置结构体初始化

  /* GPIO 端口时钟使能 */
  __HAL_RCC_GPIOB_CLK_ENABLE();              // 使能 GPIOB 时钟

  /* 配置 GPIO 输出电平为默认高电平 */
  HAL_GPIO_WritePin(GPIOB, ESP8266_IO_Pin|ESP8266_RST_Pin, GPIO_PIN_SET); // IO 和 RST 引脚置高

  /* 配置 GPIO 引脚模式 */
  GPIO_InitStruct.Pin = ESP8266_IO_Pin|ESP8266_RST_Pin; // 选择 ESP8266 IO 和 RST 引脚
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;           // 设置为推挽输出
  GPIO_InitStruct.Pull = GPIO_NOPULL;                  // 无上下拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;         // 低速模式
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);             // 初始化 GPIO 引脚
}


void ESP8266_Reset(void)
{
    // 拉低复位引脚
    HAL_GPIO_WritePin(ESP8266_RST_GPIO_Port, ESP8266_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);  // 保持低电平 100ms

    // 拉高复位引脚
    HAL_GPIO_WritePin(ESP8266_RST_GPIO_Port, ESP8266_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(500);  // 等待模块启动完成
}



void UART_StartReceive(void)
{
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, Debug_RxBuff, UART_RX_BUFF_LEN);  // UART1 使用空闲中断模式接收数据到缓冲区 Debug_RxBuff
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, ESP8266_RxBuff, UART_RX_BUFF_LEN); // UART3 使用 DMA 模式接收数据到缓冲区 ESP8266_RxBuff
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART1)  // 上位机数据到达
    {
        // 将上位机数据转发到 ESP8266
        HAL_UART_Transmit(&huart3, Debug_RxBuff, Size, HAL_MAX_DELAY);

        // 清空缓冲，防止残留
        memset(Debug_RxBuff, 0, UART_RX_BUFF_LEN);

        // 重新开启上位机 IT 接收
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, Debug_RxBuff, UART_RX_BUFF_LEN);
    }
    else if(huart->Instance == USART3)  // ESP8266 数据到达
    {
				ESP_RX_Complete = 1;
			
			if(ESP8266_TransparentMode == 1){
        // 将ESP8266数据转发到上位机
        HAL_UART_Transmit_IT(&huart1, ESP8266_RxBuff, Size);
			}

        // 重新开启 ESP8266 DMA 接收
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, ESP8266_RxBuff, UART_RX_BUFF_LEN);
    }
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART3)  // 发送到ESP8266完成
    {
        // 用 IT 发送完成，置位
        ESP_TX_Complete = 1;
    }
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        // 清除帧错误标志
        __HAL_UART_CLEAR_FEFLAG(huart);

        // 重新启动 ESP8266 DMA 接收
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, ESP8266_RxBuff, UART_RX_BUFF_LEN);
    }
}



void ESP8266_Init(void)
{
    // 复位 ESP8266 模块
    ESP8266_Reset();                        

    // 初始化 ESP8266 DMA 接收

    ESP8266_GPIO_Init();                                      

    // 启动 UART 接收任务
    UART_StartReceive();                                      
}


void ESP8266_SendString(char *str)
{
    /* 直接调用 HAL 库串口发送，阻塞方式 */
    HAL_UART_Transmit_IT(&huart3, (uint8_t *)str, strlen(str));
}


uint8_t ESP8266_Cmd(const char *cmd)
{
    // 初始化标志位
    ESP_RX_Complete = 0;    
    ESP_TX_Complete = 0;    

    memset(ESP8266_RxBuff,0,UART_RX_BUFF_LEN);
	  char majiaqi[128];
	  snprintf(majiaqi,sizeof(majiaqi),"%s\r\n",cmd);
	  ESP8266_SendString(majiaqi);
	  uint32_t time=HAL_GetTick();
	  while(!ESP_TX_Complete)
		{
			if(HAL_GetTick()-time>100)
			return 0;
			}
		return 1;
}


uint8_t ESP8266_Test(void)
{
    if(!ESP8266_Cmd("AT"))
		{
			return 0;
		}
		uint32_t time=HAL_GetTick();
		while(!ESP_RX_Complete)
		{
			if(HAL_GetTick()-time>100)
			{
				return 0;
			}
		}
		ESP8266_RxBuff[UART_RX_BUFF_LEN-1]='\0';
		if(strstr((char *)ESP8266_RxBuff,"OK")==NULL)
		{
			return 0;
			}
		return 1;
   
}

uint8_t ESP8266_SoftReset_WaitReady(void)
{
    // 发送软复位指令 AT+RST
    if(!ESP8266_Cmd("AT+RST"))
        return 0;   // 指令发送失败，返回 0

    // 获取当前系统时间
    uint32_t tick_start = HAL_GetTick();
    // 等待最大 5 秒
    while(HAL_GetTick() - tick_start < 5000)
    {
        // 判断是否接收完成
        if(ESP_RX_Complete)
        {
            // 确保接收字符串以 '\0' 结尾
            ESP8266_RxBuff[UART_RX_BUFF_LEN - 1] = '\0';

            // 检查是否包含 "ready"
            if(strstr((char *)ESP8266_RxBuff, "ready") != NULL)
            {
                ESP_RX_Complete = 0;   // 清除标志位
                return 1;              // 收到 ready，复位完成
            }

            // 未匹配到 ready，清除标志位，继续等待
            ESP_RX_Complete = 0;
        }
    }

    // 超时未收到 "ready"，返回失败
    return 0;
}


uint8_t ESP8266_SetMode(ENUM_Net_ModeTypeDef mode)
{
    char *cmd;   // 定义 AT 指令字符串指针

    // 根据传入的模式选择对应 AT 指令
    switch(mode)
    {
        case STA:    
            cmd = "AT+CWMODE=1";   // 设置为 STA 模式
            break;
        case AP:     
            cmd = "AT+CWMODE=2";   // 设置为 AP 模式
            break;
        case STA_AP: 
            cmd = "AT+CWMODE=3";   // 设置为 STA+AP 模式
            break;
        default:     
            return 0;              // 非法模式参数，返回失败
    }

    // 发送设置模式指令
    if(!ESP8266_Cmd(cmd)) 
        return 0;   // 发送失败，返回 0

    // 获取当前系统时间
    uint32_t tick_start = HAL_GetTick();
    // 等待接收完成
    while(!ESP_RX_Complete)
    {
        // 超过 1000ms 超时
        if(HAL_GetTick() - tick_start > 1000)
            return 0;   // 返回失败
    }

    // 确保字符串结尾
    ESP8266_RxBuff[UART_RX_BUFF_LEN - 1] = '\0';

    // 检查返回数据是否包含 "OK"
    if(strstr((char *)ESP8266_RxBuff, "OK"))
        return 1;   // 包含 OK，设置成功

    // 未包含 OK，返回失败
    return 0;
}


uint8_t ESP8266_SetConnMode(ENUM_ConnModeTypeDef mode)
{
    /* 定义发送指令缓冲区 */
    char *cmd;

    /* 根据模式选择指令
       SINGLE_CONN: 单连接模式
       MULTI_CONN: 多连接模式 */
    if(mode == SINGLE_CONN)
        cmd = "AT+CIPMUX=0";
    else
        cmd = "AT+CIPMUX=1";

    /* 发送设置指令
       ESP8266_Cmd 返回 0 表示发送失败 */
    if(!ESP8266_Cmd(cmd))
        return 0;

    /* 获取当前系统时间，用于超时判断 */
    uint32_t tick_start = HAL_GetTick();

    /* 等待接收完成，超时 1 秒返回失败 */
    while(!ESP_RX_Complete)
    {
        if(HAL_GetTick() - tick_start > 1000)
            return 0;
    }

    /* 确保接收缓冲字符串以 '\0' 结尾 */
    ESP8266_RxBuff[UART_RX_BUFF_LEN - 1] = '\0';

    /* 判断返回是否包含 "OK"，包含则表示设置成功 */
    if(strstr((char *)ESP8266_RxBuff, "OK") != NULL)
        return 1;

    /* 非 OK 返回失败 */
    return 0;
}


uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *password)
{
    char cmd[128];                                          // 定义发送指令缓冲区
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, password); // 构造连接 Wi-Fi 指令

    // 发送连接指令
    if(!ESP8266_Cmd(cmd)) return 4;     	// 发送失败，返回 4
	
    uint32_t tick_start = HAL_GetTick();  // 获取当前系统时间

    // 等待接收最终状态或超时（最大等待 15 秒）
    while(HAL_GetTick() - tick_start < 15000)
    {
        if(ESP_RX_Complete)
        {
            ESP8266_RxBuff[UART_RX_BUFF_LEN - 1] = '\0';

            // 连接成功
            if(strstr((char *)ESP8266_RxBuff, "OK"))
                return 0;

            // 连接失败或错误码
            if(strstr((char *)ESP8266_RxBuff, "+CWJAP:1")) return 1;  // 连接超时
            if(strstr((char *)ESP8266_RxBuff, "+CWJAP:2")) return 2;  // 密码错误
            if(strstr((char *)ESP8266_RxBuff, "+CWJAP:3")) return 3;  // 找不到目标 AP
            if(strstr((char *)ESP8266_RxBuff, "+CWJAP:4") ||
               strstr((char *)ESP8266_RxBuff, "FAIL")) return 4;      // 其他失败

            // 清标志，继续等待下一条信息
            ESP_RX_Complete = 0;
        }
    }

    // 超时未收到最终状态
    return 1;
}


uint8_t ESP8266_ConnectTCPServer(const char *ip, uint16_t port, uint8_t conn_id)
{
    /* 定义 AT 指令缓冲区 */
    char cmd[128];

    /* ESP8266 最多支持 5 个连接，编号范围 0~4
       编号非法直接返回失败 */
    if(conn_id > 4) return 0;

    /* 根据连接编号生成 AT 指令
       conn_id = 0: 单连接模式
       conn_id > 0: 多连接模式 */
    if(conn_id == 0)
        snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d", ip, port);
    else
        snprintf(cmd, sizeof(cmd), "AT+CIPSTART=%d,\"TCP\",\"%s\",%d", conn_id, ip, port);

    /* 发送 AT 指令到 ESP8266
       发送失败返回 0 */
    if(!ESP8266_Cmd(cmd))
        return 0;

    /* 获取当前时间，用于超时判断 */
    uint32_t tick_start = HAL_GetTick();

    /* 最长等待 5 秒接收响应 */
    while(HAL_GetTick() - tick_start < 5000)
    {
        /* 接收到数据标志 */
        if(ESP_RX_Complete)
        {
            /* 确保字符串以 '\0' 结尾 */
            ESP8266_RxBuff[UART_RX_BUFF_LEN - 1] = '\0';

            /* 判断是否包含 "OK"，包含表示连接成功 */
            if(strstr((char *)ESP8266_RxBuff, "OK"))
            {
                return 1;
            }

            /* 清除接收完成标志，继续等待下一条消息 */
            ESP_RX_Complete = 0;
        }
    }

    /* 超时或连接失败返回 0 */
    return 0;
}


uint8_t ESP8266_SetTransparentMode(ENUM_TransparentModeTypeDef mode)
{
    /* 定义 AT 指令缓冲区 */
    char *cmd;

    /* 根据模式生成 AT 指令
       TRANSPARENT_OFF: 关闭透传模式
       TRANSPARENT_ON:  开启透传模式 */
    if(mode == TRANSPARENT_OFF)
        cmd = "AT+CIPMODE=0";
    else
        cmd = "AT+CIPMODE=1";

    /* 发送 AT 指令
       ESP8266_Cmd 返回 0 表示发送失败 */
    if(!ESP8266_Cmd(cmd))
        return 0;

    /* 获取当前系统时间，用于超时判断 */
    uint32_t tick_start = HAL_GetTick();

    /* 等待接收完成，超时 1 秒返回失败 */
    while(!ESP_RX_Complete)
    {
        if(HAL_GetTick() - tick_start > 1000)
            return 0;
    }

    /* 确保接收缓冲字符串以 '\0' 结尾 */
    ESP8266_RxBuff[UART_RX_BUFF_LEN - 1] = '\0';

    /* 判断返回是否包含 "OK"，包含则表示设置成功 */
    if(strstr((char *)ESP8266_RxBuff, "OK") != NULL)
    {
        return 1;
    }

    /* 未返回 OK，设置失败 */
    return 0;
}


uint8_t ESP8266_SetSendMode(void)
{
    /* 发送 AT+CIPSEND 指令 */
    if(!ESP8266_Cmd("AT+CIPSEND"))
        return 0; /* 发送失败直接返回 0 */

    /* 获取当前系统时间，用于超时判断 */
    uint32_t tick_start = HAL_GetTick();

    /* 等待 ESP8266 响应完成，超时 1 秒返回失败 */
    while(!ESP_RX_Complete)
    {
        if(HAL_GetTick() - tick_start > 1000)
            return 0;
    }

    /* 确保接收缓冲字符串以 '\0' 结尾 */
    ESP8266_RxBuff[UART_RX_BUFF_LEN - 1] = '\0';

    /* 判断返回是否包含 "OK"，包含则表示成功进入发送模式 */
    if(strstr((char *)ESP8266_RxBuff, "OK") != NULL)
    {
        ESP8266_TransparentMode = 1; /* 开启 ESP8266 数据回显 */
        return 1;                    /* 返回成功 */
    }

    /* 未返回 OK，进入发送模式失败 */
    return 0;
}


