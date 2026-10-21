#ifndef __CUNCHU_H
#define __CUNCHU_H

#include "main.h"

// 结构体定义（放这里，cunchu.c 才能认识 EnvRecord_t）
typedef struct {
    uint32_t timestamp;
    float temp;
    float humi;
    float light;
} EnvRecord_t;

// 宏定义
#define FLASH_DATA_ADDR 0x001000 // 避开第0个扇区
#define RECORD_MAX_NUM  200      // 存 200 条就翻页

// 函数声明
void Flash_App_Init(void);
void Flash_SaveRecord(float temp, float humi, float light);
uint8_t Flash_ReadRecord(uint16_t index, EnvRecord_t *record);
void Flash_Task(void);
uint8_t Flash_Read(uint16_t index, EnvRecord_t *record);
uint16_t Flash_ReadAll(EnvRecord_t *buffer, uint16_t max_num);
#endif