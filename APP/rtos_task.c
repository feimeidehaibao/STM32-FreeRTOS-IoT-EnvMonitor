#include "cmsis_os.h"
#include "display.h"
#include "net.h"
#include "cunchu.h"
#include "main.h"
#include "rtos_task.h"
#include "dht11.h"
#include <stdio.h>
#include "light.h"
#include "esp8266.h"
#include "string.h"
extern DHT11_DATA_TYPEDEF dht11_data;
extern float light;
extern uint8_t Debug_RxBuff[UART_RX_BUFF_LEN];   
extern uint8_t ESP8266_RxBuff[UART_RX_BUFF_LEN]; 
extern volatile uint8_t ESP_RX_Complete; 
static void Task_Display(void const * argument);
static void Task_Comm(void const * argument);
static void Task_Storage(void const * argument);
void RTOS_Task_Init(void)
{
    // 参数说明：任务函数, 任务名, 栈大小, 传入参数, 优先级, 任务句柄
    xTaskCreate(Task_Display, "TaskDisp", 256, NULL, 2, NULL);
    xTaskCreate(Task_Comm,    "TaskComm", 512, NULL, 3, NULL);
    xTaskCreate(Task_Storage, "TaskStor", 256, NULL, 1, NULL);
}

static void Task_Display(void const * argument)
{
   for (;;)
    {
        // 1. 按键扫描
        App_Key_Scan();         
        // 2. 屏幕刷新
        oled_Show();           
        // 3. 阻塞 100 毫秒
        osDelay(100); 
    }
}

static void Task_Comm(void const * argument)
{
    ESP8266_Task(); 

    static uint32_t t_report = 0;

    for (;;)
    {
        uint32_t now = HAL_GetTick();

        // --- 1. 监听底层是否断线 ---
        if (ESP_RX_Complete == 1) 
					{
            // 检查缓冲区里有没有断线关键字
            if (strstr((char *)ESP8266_RxBuff, "CLOSED") != NULL || 
                strstr((char *)ESP8266_RxBuff, "CONNECT FAIL") != NULL) {
                
                printf(">>> 检测到服务器断开，正在重连...\r\n");
                ESP8266_Reset(); 
                ESP8266_Task();  
            }
        }

        // --- 2. 正常上报数据 ---
        if (now - t_report >= 5000) {
            t_report = now;
            ESP8266_SendSensorData_Task(); 
        }

        ESP8266_TransparentTask(); 
        osDelay(50); //阻塞五十毫秒
    }
}

static void Task_Storage(void const * argument)
{
    for (;;)
    {
        // 取数据
        float temp_float = dht11_data.temp_int + (dht11_data.temp_deci / 10.0f);
        float humi_float = dht11_data.humi_int + (dht11_data.humi_deci / 10.0f);
        
        Flash_SaveRecord(temp_float, humi_float, light); // 写入 Flash
        osDelay(60000); // 阻塞 60 秒（60000毫秒）
    }
}