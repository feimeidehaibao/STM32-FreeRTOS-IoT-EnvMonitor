#include "light.h"
#include "adc.h"

HAL_StatusTypeDef light_Get(float *data)
{
    uint16_t val = 0;
    
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        val = HAL_ADC_GetValue(&hadc1);
        *data = (float)val * 3.3f / 4095.0f; // 把算出来的结果塞进外部传来的地址里
        return HAL_OK; // 成功返回 HAL_OK
    }
    
    return HAL_ERROR; // 失败返回 HAL_ERROR，不污染外部数据
}
