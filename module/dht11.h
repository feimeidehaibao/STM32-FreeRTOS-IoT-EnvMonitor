#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"   // 必须先包含 main.h，才能用 CubeMX 生成的引脚宏

// 把你在 dht11.c 里用的全大写名字，映射到 CubeMX 生成的小写名字
#define DHT11_GPIO_PORT   dht11_GPIO_Port
#define DHT11_GPIO_PIN    dht11_Pin

// ... 下面是你原有的函数声明 ...
void DHT11_Set_Output(void);
void DHT11_Set_Input(void);
void DHT11_Start(void);


// 把5个字节打包成一个结构体
typedef struct {
    uint8_t humi_int;    // 湿度整数
    uint8_t humi_deci;   // 湿度小数
    uint8_t temp_int;    // 温度整数
    uint8_t temp_deci;   // 温度小数
    uint8_t check_sum;   // 校验和
} DHT11_DATA_TYPEDEF;

// 函数声明：传入结构体指针
HAL_StatusTypeDef DHT11_Get(DHT11_DATA_TYPEDEF *data);

#endif