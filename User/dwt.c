#include "dwt.h"

void DWT_Init(void)
{
    // 1. 使能跟踪单元（内核调试组件）
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    // 2. 清零周期计数器
    DWT->CYCCNT = 0;
    
    // 3. 使能周期计数器（让它在后台一直累加）
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t DWT_GetTick(void)
{
    return DWT->CYCCNT;
}

void DWT_Delay_us(uint32_t us)
{
    // 1. 获取起始计数值
    uint32_t start_tick = DWT->CYCCNT;
    
    // 2. 计算需要等待的周期数
    // SystemCoreClock 目前是 72000000 (72MHz)
    // 1 微秒 = 72 个时钟周期
    uint32_t target_ticks = us * (SystemCoreClock / 1000000);
    
    // 3. 循环等待，直到计数值之差超过目标周期数
    // 注意：这里依靠的是硬件寄存器不断累加，不依赖中断
    while ((DWT->CYCCNT - start_tick) < target_ticks);
}

void DWT_Delay_ms(uint32_t us)
{
	for(uint32_t i=0;i<us;i++)
	{
		DWT_Delay_us(1000);
	}
}

void DWT_Delay_s(uint32_t us)
{
	for(uint32_t i=0;i<us;i++)
	{
		DWT_Delay_ms(1000);
	}
}