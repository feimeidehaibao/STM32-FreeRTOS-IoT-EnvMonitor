#include "test.h"
#include "dht11.h"             // Module 层
#include "flash.h"    // Module 层
#include <string.h>
#include <stdio.h>
// 测试用的 Flash 读写缓存
uint8_t test_tx_buf[256] = {0};
uint8_t test_rx_buf[256] = {0};

void App_Test(void)
{
    // ==========================================
    // 测试 2：W25Q64 Flash
    // ==========================================
    printf("========== 开始测试 W25Q64 ==========\r\n");
    
    // 1. 读取 JEDEC ID 确认芯片在线
    uint32_t id = SPI_FLASH_JEDEC_ID();
    printf("JEDEC ID: 0x%X\r\n", id);
    if (id != 0xEF4017) // W25Q64 的标准 ID
    {
        printf("Flash 未检测到或 ID 错误！停止测试。\r\n");
        return; // 如果 ID 不对，后面的擦写测试就没必要做了
    }
    
    // 2. 准备测试数据 (写入 0x00 ~ 0xFF)
    for (uint16_t i = 0; i < 256; i++)
    {
        test_tx_buf[i] = (uint8_t)i;
    }

    // 3. 扇区擦除 (测试地址 0x000000)
    printf("正在擦除扇区 0x000000...\r\n");
    SPI_FLASH_SectorErase(0x000000);
    
    // 4. 写入数据
    printf("正在写入测试数据...\r\n");
    SPI_FLASH_WriteBuffer(0x000000, test_tx_buf, 256);

    // 5. 读取数据
    printf("正在读取测试数据...\r\n");
    SPI_FLASH_ReadBuffer(0x000000, test_rx_buf, 256);

    // 6. 数据校验
    if (memcmp(test_tx_buf, test_rx_buf, 256) == 0)
    {
        printf("W25Q64 读写测试成功！数据完全匹配。\r\n");
    }
    else
    {
        printf("W25Q64 数据校验失败！读写内容不一致。\r\n");
    }
    
    printf("========== 测试结束 ==========\r\n");
}