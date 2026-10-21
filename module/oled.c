/**
  ******************************************************************************
  * @file       oled.c
  * @author     embedfire (底层驱动原始框架)
  * @modifier   Yang Chenghao (杨承澔) 
  ******************************************************************************
  */
  
#include "oled.h"
#include "i2c.h"
#include "usart.h"
#include "stdio.h"
#include "font.h"
#include <string.h>
static uint8_t i2c_fail_count = 0;
  
	
	
ErrorStatus OLED_CheckDevice(uint8_t slave_addr)
{
    uint8_t dummy = 0;
    
    // 检测通信
    if (HAL_I2C_Master_Transmit(&hi2c1, (slave_addr << 1), &dummy, 0, IIC_TIMEOUT) == HAL_OK)
    {
        return SUCCESS;
    }
    
    return ERROR;
}


ErrorStatus OLED_WriteByte(uint8_t cmd, uint8_t byte)
{
    uint8_t buf[2] = {cmd, byte};  // 先放控制字节，再放要写的数据，形成2字节发送数据包

    // 通过I2C主机发送数据包给OLED设备，地址左移1位为7位地址格式，发送2个字节，超时为IIC_TIMEOUT
    if (HAL_I2C_Master_Transmit(&hi2c1, OLED_SLAVER_ADDR << 1, buf, 2, IIC_TIMEOUT) == HAL_OK)
        return SUCCESS;            // 发送成功，返回成功状态
     i2c_fail_count++;
    if (i2c_fail_count > 5) { // 如果连续失败超过 5 次
        i2c_fail_count = 0;
        printf(">>> I2C 总线死锁，正在执行硬复位抢救！\r\n");
        
        // 1. 强行复位 I2C 外设
        HAL_I2C_DeInit(&hi2c1);
        HAL_I2C_Init(&hi2c1);
        
        // 2. 重新初始化 OLED
        OLED_Init(); 
    }
    // 发送失败，打印错误信息，方便调试
    printf("OLED_WriteByte 写入失败：cmd=0x%02X byte=0x%02X\r\n", cmd, byte);
    return ERROR;                  // 返回失败状态
}


ErrorStatus OLED_WriteBuffer(uint8_t cmd, uint8_t *buffer, uint32_t num)
{
    // 改成 static，避免栈溢出
    static uint8_t majiaqi[256]; 
    
    if(num > sizeof(majiaqi) - 1)
    {

        return ERROR;
    }
    
    majiaqi[0] = cmd;
    memcpy(&majiaqi[1], buffer, num);
    
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, OLED_SLAVER_ADDR << 1, majiaqi, num + 1, IIC_TIMEOUT);
    
    if(status == HAL_OK)
    {
        return SUCCESS;
    }
    
    return ERROR;
}


void OLED_SetPos(uint8_t y, uint8_t x)
{
	uint8_t majiaqi[3]={0xB0+y,(x&0xF0)>>4|0x10,x&0x0F};
	OLED_WriteBuffer(OLED_WR_CMD, majiaqi, sizeof(majiaqi));

}


void OLED_Fill(uint8_t fill_data)
{
    uint8_t data_buffer_temp[128] = {0};
    // 将数组的128个字节全部设置为 fill_data，准备写入整行数据
    memset(data_buffer_temp, fill_data, 128);

    // OLED共有8页，每页128列，循环逐页写入数据
    for(uint8_t m = 0; m < 8; m++)
    {
        OLED_SetPos(m, 0); // 设置光标位置到第m页第0列
        OLED_WriteBuffer(OLED_WR_DATA, data_buffer_temp, OLED_ARRAY_SIZE(data_buffer_temp)); // 写入整页数据
    }
}


void OLED_CLS(void)
{
    OLED_Fill(0x00);  // 填充 0x00，所有像素点灭，清空屏幕显示内容
}


void OLED_FillFull(void)
{
    OLED_Fill(0xFF);  // 填充 0xFF，所有像素点亮，屏幕亮满
}

/**
  * @brief  OLED 初始化
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{
    HAL_Delay(100);  // 延时保证电源稳定

    while (OLED_CheckDevice(OLED_SLAVER_ADDR) != SUCCESS);

		/* 控制显示 */
		OLED_WriteByte(OLED_WR_CMD,0xAE);//设置显示打开/关闭(AFh/AEh)

		/* 控制内存寻址模式 */
		OLED_WriteByte(OLED_WR_CMD,0x20);//设置内存寻址模式(20h)
		OLED_WriteByte(OLED_WR_CMD,0x02);//00b，水平寻址模式;01b，垂直寻址模式;10b，页面寻址模式(RESET);11，无效

		/* 页起始地址 */ 
		OLED_WriteByte(OLED_WR_CMD,0xB0);//设置页面寻址模式的页面起始地址，0-7(B0-B7)(PAGE0-PAGE7)

		/* COM输出扫描方向 */
		OLED_WriteByte(OLED_WR_CMD,0xA1);//设置左右方向，0xA1正常 0xA0左右反置
		OLED_WriteByte(OLED_WR_CMD,0xC8);//设置上下方向，0xC8正常 0xC0上下反置

		/* 页内列起始地址 */
		OLED_WriteByte(OLED_WR_CMD,0x00);//设置列地址低位0-7
		OLED_WriteByte(OLED_WR_CMD,0x10);//设置列地址高位0-F(10h-1Fh) 列序号=列地址低位*列地址高位(最高位不参与乘积)

		/* 页内行起始地址 */
		OLED_WriteByte(OLED_WR_CMD,0x40);//设置起始行地址

		/* 设置对比度 */
		OLED_WriteByte(OLED_WR_CMD,0x81);//设置对比度控制寄存器(81h)
		OLED_WriteByte(OLED_WR_CMD,0xff);//0x00~0xff

		/* 设置显示方向 */
		OLED_WriteByte(OLED_WR_CMD,0xA1);//将列地址0映射到SEG0(A0h)/将列地址127映射到SEG0(A1h)
		OLED_WriteByte(OLED_WR_CMD,0xA6);//设置正常显示(A6h)/倒转显示(A7h)

		/* 设置多路复用率 */
		OLED_WriteByte(OLED_WR_CMD,0xA8);//多路复用率(A8h)
		OLED_WriteByte(OLED_WR_CMD,0x3F);//(1 ~ 64)

		/* 全屏显示 */
		OLED_WriteByte(OLED_WR_CMD,0xA4);//设置整个显示打开/关闭(A4恢复到RAM内容显示,输出遵循RAM内容/A5全屏显示,输出忽略RAM内容)

		/*设置显示偏移量*/
		OLED_WriteByte(OLED_WR_CMD,0xd3);//设置显示偏移量(D3h)
		OLED_WriteByte(OLED_WR_CMD,0x00);

		/* 设置显示时钟分频比/振荡器频率 */
		OLED_WriteByte(OLED_WR_CMD,0xD5);//设置显示时钟分频比/振荡器频率
		OLED_WriteByte(OLED_WR_CMD,0xf0);//设定分割比

		/* 设置预充期 */
		OLED_WriteByte(OLED_WR_CMD,0xD9);//设置预充期
		OLED_WriteByte(OLED_WR_CMD,0x22);

		/* 设置com引脚硬件配置 */
		OLED_WriteByte(OLED_WR_CMD,0xDA);//设置com引脚硬件配置
		OLED_WriteByte(OLED_WR_CMD,0x12);

		/* 设置VCOMH取消选择级别 */
		OLED_WriteByte(OLED_WR_CMD,0xDB);//设置VCOMH取消选择级别
		OLED_WriteByte(OLED_WR_CMD,0x20);//0x20,0.77xVcc

		OLED_WriteByte(OLED_WR_CMD,0x8D);//设置DC-DC使能
		OLED_WriteByte(OLED_WR_CMD,0x14);

		/* 显示打开 */
		OLED_WriteByte(OLED_WR_CMD,0xAF);//设置显示打开/关闭(AFh/AEh)

		OLED_CLS();     // 初始化完成后清屏，确保屏幕全黑，清除残留显示内容

}


void OLED_ShowChinese(uint8_t y, uint8_t x, uint8_t n, const uint8_t data_cn[][16])
{
    OLED_SetPos(y, x);  // 设置到第一页位置
    for (uint8_t i = 0; i < 16; i++)  // 写入上半部分 16 字节
    {
        OLED_WriteByte(OLED_WR_DATA, data_cn[n * 2][i]);
    }

    OLED_SetPos(y + 1, x);  // 设置到第二页位置
    for (uint8_t i = 0; i < 16; i++)  // 写入下半部分 16 字节
    {
        OLED_WriteByte(OLED_WR_DATA, data_cn[n * 2 + 1][i]);
    }
}


void OLED_ShowChinese_F16X16(uint8_t line, uint8_t offset, uint8_t n)
{
    OLED_ShowChinese(line * 2, offset * 16, n, chinese_library_16x16);
}


void OLED_ShowChar(uint8_t y, uint8_t x, uint8_t char_data, uint8_t textsize)
{
    uint32_t addr = char_data - 32;  // 计算字符在点阵数组中的索引

    switch(textsize)
    {
        case TEXTSIZE_F6X8:   // 6x8点阵字体，宽6像素，高8像素
            OLED_SetPos(y, x);  // 设置光标位置
            for(uint8_t i = 0; i < 6; i++)  // 依次写入6字节点阵数据
            {
                OLED_WriteByte(OLED_WR_DATA, (uint8_t)ascii_code_6x8[addr][i]);
            }
            break;

        case TEXTSIZE_F8X16:  // 8x16点阵字体，宽8像素，高16像素
            OLED_SetPos(y, x);  // 设置光标位置显示上半部分
            for(uint8_t i = 0; i < 8; i++)  // 写入8字节点阵数据（上半部分）
            {
                OLED_WriteByte(OLED_WR_DATA, (uint8_t)ascii_code_8x16[addr][i]);
            }
            OLED_SetPos(y + 1, x);  // 设置下一页光标显示下半部分
            for(uint8_t i = 0; i < 8; i++)  // 写入剩余8字节点阵数据（下半部分）
            {
                OLED_WriteByte(OLED_WR_DATA, (uint8_t)ascii_code_8x16[addr][i + 8]);
            }
            break;

        default:
            break;
    }
}


void OLED_ShowString(uint8_t y, uint8_t x, uint8_t *string_data, uint8_t textsize)
{
    for(uint8_t i = 0; *string_data != '\0'; i++)
    {
				// 逐字符调用 OLED_ShowChar，x坐标根据字符宽度递增，实现横向连续显示
        OLED_ShowChar(y, x + i * textsize, *string_data++, textsize);
    }
}


void OLED_ShowString_F8X16(uint8_t line, uint8_t offset, uint8_t *string_data)
{
    OLED_ShowString(line * 2, offset * 8, string_data, TEXTSIZE_F8X16);
}





