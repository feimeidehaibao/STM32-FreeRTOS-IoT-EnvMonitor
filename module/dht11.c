#include "dht11.h"
#include "dwt.h"  // 必须用到我们刚写的DWT微秒延时

// 1. 把引脚切为推挽输出模式
void DHT11_Set_Output(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin=DHT11_GPIO_PIN;
	GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull=GPIO_PULLUP;
	GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

// 2. 把引脚切为输入模式（带内部上拉）
void DHT11_Set_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;     // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;         // 弱上拉，确保空闲时为高电平
    HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

// 3. 发起始信号 (STM32 敲门)
void DHT11_Start(void)
{
    DHT11_Set_Output(); // 切输出
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET); // 拉低
    DWT_Delay_ms(20);   // 保持至少18ms（用我们前面写的DWT，虽然有点误差但完全够用）
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);   // 释放（拉高）
    DWT_Delay_us(30);   // 等待30us，让DHT11准备响应
    
    DHT11_Set_Input();  // 立刻切成输入，准备听DHT11的回复
}

// 读取一个字节（8位）
uint8_t DHT11_Read_Byte(void)
{
    uint8_t data = 0;
    uint32_t start_tick, time;
    
    for (int i = 0; i < 8; i++)
    {
        // 1. 等待50us低电平结束（引脚变高）
        // 这里需要一个超时死等，防卡死。为了初学者逻辑清晰，我们先给一个简单的超时
        uint32_t timeout = 0;
        while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_RESET) {
            if (++timeout > 1000) return 0; // 超时退出
        }
        
        // 2. 记录高电平起始时间
        start_tick = DWT_GetTick();
        
        // 3. 等待高电平结束（引脚变低）
        timeout = 0;
        while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET) {
            if (++timeout > 1000) return 0; // 超时退出
        }
        
        // 4. 计算高电平持续时间（单位：时钟周期，72个周期=1us）
        time = DWT_GetTick() - start_tick;
        
        // 5. 移位与赋值
        data <<= 1; // 先左移
        if (time > 40 * 72) { // 如果大于40us（乘以72换算成周期数）
            data |= 1;        // 是 1
        }
        // 否则就是 0，什么都不用做（左移后低位默认是0）
    }
    return data;
}

HAL_StatusTypeDef DHT11_Get(DHT11_DATA_TYPEDEF *data)
{
    uint8_t retry = 0;

    // 1. 主机发起始信号（拉低至少18ms）
    DHT11_Set_Output(); // 切换为推挽输出
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
    DWT_Delay_ms(20);   // 保持低电平20ms
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
    DWT_Delay_us(30);   // 释放总线，等30us

    // 2. 切换为输入模式，准备接收
    DHT11_Set_Input(); 

    // 3. 三次握手（等待DHT11的响应信号）—— 这里是野火的核心精髓
    // 等待拉低（响应信号开始，约83us）
    retry = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET) {
        if (++retry > 100) return HAL_ERROR;
        DWT_Delay_us(1);
    }

    // 等待拉高（响应信号结束，约87us）
    retry = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_RESET) {
        if (++retry > 100) return HAL_ERROR;
        DWT_Delay_us(1);
    }

    // 等待再次拉低（数据位起点，50us低电平）
    retry = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET) {
        if (++retry > 100) return HAL_ERROR;
        DWT_Delay_us(1);
    }

    // 4. 连续读取5个字节，通过指针直接写入外部的结构体
    data->humi_int  = DHT11_Read_Byte();
    data->humi_deci = DHT11_Read_Byte();
    data->temp_int  = DHT11_Read_Byte();
    data->temp_deci = DHT11_Read_Byte();
    data->check_sum = DHT11_Read_Byte();

    // 5. 校验：前4个字节的和是否等于第5个字节
    uint8_t sum = data->humi_int + data->humi_deci + data->temp_int + data->temp_deci;
    if (sum != data->check_sum) {
        return HAL_ERROR; // 校验失败，数据不可信
    }

    return HAL_OK; // 读取成功
}