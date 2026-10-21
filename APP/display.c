/**
  ******************************************************************************
  * @file       display.c
  * @author     应用层
  * @modifier   Yang Chenghao (杨承澔) 
  参考了野火官方的温湿度显示代码，并在加上了光照强度显示代码和切换显示模式代码
  ******************************************************************************
  */
#include "display.h"
#include "dht11.h" 
#include "dwt.h"
#include "oled.h"
#include <stdio.h>
#include "light.h"
#include "gpio.h"
DHT11_DATA_TYPEDEF dht11_data = {0};
float light = 0.0f;
 uint8_t stat = 0;
uint32_t t_display = 0;
  uint32_t t_key = 0;

void OLED_dht11(void)
{
	  OLED_CLS();
    
    OLED_ShowChinese_F16X16(0, 0, 0);  
    OLED_ShowChinese_F16X16(0, 1, 1);  
    OLED_ShowChinese_F16X16(0, 2, 2);  
    OLED_ShowChinese_F16X16(0, 3, 3);  
	  OLED_ShowChinese_F16X16(0, 4, 4);
	  OLED_ShowChinese_F16X16(0, 5, 5);
	  OLED_ShowChinese_F16X16(0, 6, 6); //第一行显示“温湿度显示模式”
    if(DHT11_Get(&dht11_data) == HAL_OK)
    {

        OLED_ShowChinese_F16X16(1, 0, 0);  // 第1行 第0列：显示“温”
        OLED_ShowChinese_F16X16(1, 1, 2);  // 第1行 第1列：显示“度”
        OLED_ShowString_F8X16(1, 4, (uint8_t *)":");  // 显示冒号“:”
	
        char temp_str[8];
        sprintf(temp_str, "%2d.%1dC", dht11_data.temp_int, dht11_data.temp_deci);  // 格式化温度字符串，如“27.0C”
        OLED_ShowString_F8X16(1, 6, (uint8_t *)temp_str);  // 显示温度数值
			
        OLED_ShowChinese_F16X16(2, 0, 1);  // 第2行 第0列：显示“湿”
        OLED_ShowChinese_F16X16(2, 1, 2);  // 第2行 第1列：显示“度”
        OLED_ShowString_F8X16(2, 4, (uint8_t *)":");  // 显示冒号“:”

        char humi_str[8];
        sprintf(humi_str, "%2d.%1d%%", dht11_data.humi_int, dht11_data.humi_deci);  // 格式化湿度字符串，如“66.2%”
        OLED_ShowString_F8X16(2, 6, (uint8_t *)humi_str);  // 显示湿度数值
		}
		else
		{
        /* ---------- 错误提示（传感器读取失败） ---------- */
        OLED_ShowString_F8X16(1, 5, (uint8_t *)"DHT11");     // 第1行 显示“DHT11”
        OLED_ShowString_F8X16(2, 4, (uint8_t *)"ERROR!!");   // 第2行 显示“ERROR!!”
		}

    
}



void OLED_light(void)
{
	  OLED_CLS();
	  OLED_ShowChinese_F16X16(0, 0, 7);  
    OLED_ShowChinese_F16X16(0, 1, 8);  
    OLED_ShowChinese_F16X16(0, 2, 9);  
    OLED_ShowChinese_F16X16(0, 3, 10);  
	  OLED_ShowChinese_F16X16(0, 4, 3);
	  OLED_ShowChinese_F16X16(0, 5, 4);
	  OLED_ShowChinese_F16X16(0, 6, 5); 
	  OLED_ShowChinese_F16X16(0, 7, 6); //第一行显示“光照强度显示模式”
	  if(light_Get(&light)==HAL_OK)
		{
			OLED_ShowChinese_F16X16(1, 0, 7);
			OLED_ShowChinese_F16X16(1, 1, 8);
			OLED_ShowChinese_F16X16(1, 2, 9);
			OLED_ShowChinese_F16X16(1, 3, 10);//第二行显示光照强度
			OLED_ShowString_F8X16(1, 4, (uint8_t *)":");
			char majiaqi[8];
			sprintf(majiaqi,"%.2fV",light);
			OLED_ShowString_F8X16(1, 5, (uint8_t *)majiaqi);
		}
		else
		{
        /* ---------- 错误提示（传感器读取失败） ---------- */
        OLED_ShowString_F8X16(1, 5, (uint8_t *)"light");     // 第1行 显示“DHT11”
        OLED_ShowString_F8X16(2, 4, (uint8_t *)"ERROR!!");   // 第2行 显示“ERROR!!”
		}
}

void oled_qiehuan(void)
{

    
    // 根据状态显示
    if (stat == 0) 
    {
        OLED_dht11();
    } 
    else 
    {
        OLED_light();
    }
}

void App_Key_Scan(void)
{
    static uint32_t last_tick = 0;
    static uint8_t last_key_state = 0; 
    
    uint32_t now = HAL_GetTick();
    //这是消抖
    // 1. 函数内部自己控制 20ms 的时间片
    if (now - last_tick < 20) {
        return; // 还没到20ms，直接退出
    }
    last_tick = now; // 到了20ms，更新时间戳
    
    // 2. 以下是按键的边沿检测逻辑
    uint8_t current_key_state = HAL_GPIO_ReadPin(GPIOC, KEY_Pin);
    if (current_key_state == GPIO_PIN_SET && last_key_state == GPIO_PIN_RESET)
    {
        stat = !stat;
    }
    last_key_state = current_key_state;
    
}

void oled_Show(void)
{
 uint32_t now = HAL_GetTick();
   App_Key_Scan();
   

    // 2. 屏幕每 200ms 刷新一次（画图）
    if (now - t_display >= 2000) {
        t_display = now;
        oled_qiehuan(); 
    }
  }

