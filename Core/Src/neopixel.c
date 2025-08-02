/*
 * neopixel.cpp
 *
 *  Created on: Aug 26, 2024
 *      Author: kyoro
 */


#include "neopixel.h"
#include "string.h"

#define LED_NUM 10
const int RESET_NUM=100;

uint16_t LED_data[LED_NUM][3]={0};
uint32_t rgb[100+LED_NUM*3*8]={0};

void SetNeoPixel(void)
{
    const uint16_t PWM_H = 9;  // HIGHパルス幅（"1"を表す） 例: 9/12
    const uint16_t PWM_L = 3;  // LOWパルス幅（"0"を表す）  例: 3/12
    const uint16_t PWM_RESET = 0;

    memset(rgb, 0, sizeof(rgb));  // すべて0で初期化

    // リセット期間（最低50us）確保：PWM_RESET値を100個分
    for (int i = 0; i < RESET_NUM; i++) {
        rgb[i] = PWM_RESET;
    }

    // 各LEDについてGRB順にデータをビット展開
    for (int i = 0; i < LED_NUM; i++) {
        for (int k = 7; k >= 0; k--) {
            // G（1番目）
            rgb[RESET_NUM + i * 24 + (7 - k)       ] = ((LED_data[i][1] >> k) & 0x01) ? PWM_H : PWM_L;
            // R（2番目）
            rgb[RESET_NUM + i * 24 + (7 - k) +  8  ] = ((LED_data[i][0] >> k) & 0x01) ? PWM_H : PWM_L;
            // B（3番目）
            rgb[RESET_NUM + i * 24 + (7 - k) + 16  ] = ((LED_data[i][2] >> k) & 0x01) ? PWM_H : PWM_L;
        }
    }

    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)rgb, RESET_NUM + 24 * LED_NUM);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
}
void UpdateNeoPixel(uint16_t r, uint16_t g, uint16_t b)
{
    memset(LED_data, 0, sizeof(LED_data));
    for (int i = 0; i < LED_NUM; i++) {
        LED_data[i][0] = r;
        LED_data[i][1] = g;
        LED_data[i][2] = b;
    }
    SetNeoPixel();
}



