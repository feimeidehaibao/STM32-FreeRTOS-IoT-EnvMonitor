#ifndef __DWT_H
#define __DWT_H

#include "stm32f1xx_hal.h" // 核心库包含了内核外设的定义

// 初始化DWT计数器
void DWT_Init(void);

// 获取当前时间戳（单位：CPU时钟周期，1/72微秒）
uint32_t DWT_GetTick(void);
void DWT_Delay_ms(uint32_t us);
// 微秒级延时
void DWT_Delay_us(uint32_t us);
void DWT_Delay_s(uint32_t us);
#endif