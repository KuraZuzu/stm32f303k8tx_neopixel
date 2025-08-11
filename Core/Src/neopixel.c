#include "neopixel.h"
#include "string.h"

#define LED_NUM 10

// タイマ条件: 40MHz / (PSC=4) = 10MHz → 0.1us/tick, ARR=11 → 12tick = 1.2us
// 推奨パルス: 1のHigh=8tick(0.8us), 0のHigh=3tick(0.3us)
#define TICKS_ONE   8
#define TICKS_ZERO  3
#define RESET_TAIL  100   // 100 * 1.2us = 120us (>50us)

// GRB順（WS2812）
static uint16_t LED_data[LED_NUM][3] = {0};
static uint16_t pwm_buf[LED_NUM * 24 + RESET_TAIL];  // ★16bit & 末尾リセット

void SetNeoPixel(void)
{
    // データ部をGRBで並べる
    int idx = 0;
    for (int i = 0; i < LED_NUM; i++) {
        uint8_t g = (uint8_t)LED_data[i][1];
        uint8_t r = (uint8_t)LED_data[i][0];
        uint8_t b = (uint8_t)LED_data[i][2];

        for (int k = 7; k >= 0; k--) pwm_buf[idx++] = (g >> k) & 1 ? TICKS_ONE : TICKS_ZERO;
        for (int k = 7; k >= 0; k--) pwm_buf[idx++] = (r >> k) & 1 ? TICKS_ONE : TICKS_ZERO;
        for (int k = 7; k >= 0; k--) pwm_buf[idx++] = (b >> k) & 1 ? TICKS_ONE : TICKS_ZERO;
    }

    // 末尾にリセット(完全Low)を確保
    for (int i = 0; i < RESET_TAIL; i++) pwm_buf[idx++] = 0;

    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwm_buf, idx);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);  // ★アイドルLowなので停止後も安全
}

void UpdateNeoPixel(uint16_t r, uint16_t g, uint16_t b)
{
    // 0～255にクリップ（うっかり9～10bit来ても8bitで送るため）
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    for (int i = 0; i < LED_NUM; i++) {
        LED_data[i][0] = r;
        LED_data[i][1] = g;
        LED_data[i][2] = b;
    }
    SetNeoPixel();
}
