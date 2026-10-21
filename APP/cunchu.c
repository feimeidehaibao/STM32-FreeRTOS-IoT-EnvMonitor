#include "cunchu.h"
#include "flash.h" 
#include <stdio.h>
#include <string.h>

// 仅从第1个扇区开始存数据
#define FLASH_DATA_ADDR 0x001000 


// 存 200 条就换扇区
#define RECORD_MAX_NUM  200      

static uint16_t record_index = 0; // 检测当前存到第几条了

void Flash_App_Init(void)
{

    record_index = 0;
    printf(">>> Flash 应用层初始化完成\r\n");
}

void Flash_SaveRecord(float temp, float humi, float light)
{
    EnvRecord_t jiaqi;
	  jiaqi.timestamp=HAL_GetTick();
	  jiaqi.temp=temp;
	  jiaqi.humi=humi;
	  jiaqi.light=light;
	  uint32_t addr=FLASH_DATA_ADDR+(record_index*sizeof(EnvRecord_t));
	  if(record_index==0)
		{
			SPI_FLASH_SectorErase(FLASH_DATA_ADDR);
			printf("\r\n 已狠狠擦除 \r\n");
		}
		SPI_FLASH_WriteBuffer(addr,(uint8_t*)&jiaqi,sizeof(EnvRecord_t));
		printf("\r\n 以狠狠写入 \r\n");
		record_index++;
		 if (record_index >= RECORD_MAX_NUM) {
        record_index = 0; 
    }
}
//以下是从flash中读取数据的代码，当前工程没有用到//
uint8_t Flash_ReadRecord(uint16_t index, EnvRecord_t *record)
{
    if (index >= RECORD_MAX_NUM) return 0;
    uint32_t addr = FLASH_DATA_ADDR + (index * sizeof(EnvRecord_t));
    SPI_FLASH_ReadBuffer(addr, (uint8_t *)record, sizeof(EnvRecord_t));
    return 1;
}

void Flash_Task(void)
{
    uint32_t id_read;


    // 1. 读取 JEDEC ID，确认设备连接正常
    id_read = SPI_FLASH_JEDEC_ID();
    printf("JEDEC ID: 0x%X\r\n", id_read);
    if (id_read != 0xEF4017) 
    {
        printf("设备ID校验失败，ID=0x%X\r\n", id_read);
        return;
    }

    // 2. 随便擦除一个扇区，测试读写
    SPI_FLASH_SectorErase(0x001000); 
    uint8_t tx[4] = {0x12, 0x34, 0x56, 0x78};
    uint8_t rx[4] = {0};
    SPI_FLASH_WriteBuffer(0x001000, tx, 4);
    SPI_FLASH_ReadBuffer(0x001000, rx, 4);
    
    if (rx[0] == 0x12 && rx[1] == 0x34) {
        printf("W25Q64 读写测试成功！\r\n");
    } else {
        printf("W25Q64 读写测试失败！\r\n");
    }
}


/**
  * @brief  读取所有历史记录（可用于断网续传）
  * @param  buffer: 接收数据的数组指针
  * @param  max_num: 数组最多能装多少条
  * @retval 实际读出的条数
  */
uint16_t Flash_ReadAll(EnvRecord_t *buffer, uint16_t max_num)
{
    uint16_t read_count = (record_index < max_num) ? record_index : max_num;
    
    for (uint16_t i = 0; i < read_count; i++) {
        Flash_ReadRecord(i, &buffer[i]);
    }
    
    return read_count;
}