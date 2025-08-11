#include "neopixel.h"
#include <string.h>

/* ================== ユーザ調整セクション ================== */

/* LED 本数 */
#ifndef NEOPIXEL_LED_NUM
#define NEOPIXEL_LED_NUM   20  // ダメージパネル内のLEDの数が不明なので20に設定(10だと少ない)
#endif

/* リセット（ラッチ）期間として送る 0 パルス個数（1個=1bit期間） */
#ifndef NEOPIXEL_RESET_TAIL
#define NEOPIXEL_RESET_TAIL 100   // ≈ 100 * 1.25us = 125us
#endif

/* GRB順（WS2812系）固定：必要ならここで順序入れ替え可 */
#define ORDER_G 1
#define ORDER_R 0
#define ORDER_B 2

/* --- タイミング指定モード ---
 * 0: 固定値（CubeMXのPSC/ARRを 800kHz に合わせる運用）
 * 1: 自動（実 ARR から 1bit 中の High tick を再計算）※タイマ設定は変更しない
 */
#ifndef NEOPIXEL_USE_AUTOTIMING
#define NEOPIXEL_USE_AUTOTIMING 0
#endif

/* 固定値モード用：タイマが 40MHz, PSC=0, ARR=49（=1.25us）を前提 */
#if (NEOPIXEL_USE_AUTOTIMING == 0)
#ifndef NEOPIXEL_TICKS_ONE
#define NEOPIXEL_TICKS_ONE   28   // ≈ 0.70us (28/40MHz)
#endif
#ifndef NEOPIXEL_TICKS_ZERO
#define NEOPIXEL_TICKS_ZERO  14   // ≈ 0.35us (14/40MHz)
#endif
#endif

/* ========================================================== */

static uint16_t LED_data[NEOPIXEL_LED_NUM][3] = {0};

/* 送信用 PWM バッファ：各ビット 1 要素。末尾にリセット期間を追加 */
static uint16_t s_pwm_buf[NEOPIXEL_LED_NUM * 24 + NEOPIXEL_RESET_TAIL];

/* 自動タイミング用の実行時 tick 値（固定値モードでは定数にフォールバック） */
#if (NEOPIXEL_USE_AUTOTIMING != 0)
static uint16_t s_ticks_one  = 0;
static uint16_t s_ticks_zero = 0;
#else
enum { s_ticks_one  = NEOPIXEL_TICKS_ONE,
       s_ticks_zero = NEOPIXEL_TICKS_ZERO };
#endif

/* 内部ユーティリティ：8bit → ビット列を PWM tick に展開 */
static inline void push_byte_as_pwm(uint8_t v, volatile uint16_t *buf, int *idx)
{
    for (int k = 7; k >= 0; k--) {
        uint16_t ticks = ((v >> k) & 1U) ? s_ticks_one : s_ticks_zero;
        buf[(*idx)++] = ticks;
    }
}

/* ================== 公開 API ================== */

void NeoPixel_Init(void)
{
#if (NEOPIXEL_USE_AUTOTIMING != 0)
    /* 「自動」モード:
     * - タイマの 1bit 期間 = (ARR + 1) カウント
     * - WS2812 の中心値（T1H ≈ 0.7, T0H ≈ 0.35）を比率で算出
     * - ここでは PSC/ARR を変更しません（CubeMX設定そのまま）
     */
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
    uint32_t N   = arr + 1U;                // 1bit 内の総カウント

    /* 丸め誤差を抑えるため 1/1000 固定小数点で算出 */
    s_ticks_one  = (uint16_t)((N * 700U + 500U) / 1000U);  // ≈ 0.70 * N
    s_ticks_zero = (uint16_t)((N * 350U + 500U) / 1000U);  // ≈ 0.35 * N

    /* 下限・上限の軽いガード（0 と N-1 の範囲に収める） */
    if (s_ticks_one >= N)  s_ticks_one  = (uint16_t)(N - 1U);
    if (s_ticks_zero >= N) s_ticks_zero = (uint16_t)(N - 1U);
    if (s_ticks_one == 0)  s_ticks_one  = 1;
    if (s_ticks_zero == 0) s_ticks_zero = 1;
#endif
}

void SetNeoPixel(void)
{
    int idx = 0;

    /* GRB 順で各 LED を展開 */
    for (int i = 0; i < NEOPIXEL_LED_NUM; i++) {
        uint8_t g = (uint8_t)(LED_data[i][ORDER_G] & 0xFF);
        uint8_t r = (uint8_t)(LED_data[i][ORDER_R] & 0xFF);
        uint8_t b = (uint8_t)(LED_data[i][ORDER_B] & 0xFF);

        push_byte_as_pwm(g, s_pwm_buf, &idx);
        push_byte_as_pwm(r, s_pwm_buf, &idx);
        push_byte_as_pwm(b, s_pwm_buf, &idx);
    }

    /* 末尾リセット期間（完全 Low）を付与 */
    for (int i = 0; i < NEOPIXEL_RESET_TAIL; i++) {
        s_pwm_buf[idx++] = 0;
    }

    /* DMA HalfWord（16bit）整合: tim.c 側は Mem/Periph = HALFWORD に設定しておくこと */
    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)s_pwm_buf, idx);
}

/* DMA 転送完了で停止（アイドル Low を想定：OCIdleState=RESET） */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim1) {
        HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
    }
}

void UpdateNeoPixel(uint16_t r, uint16_t g, uint16_t b)
{
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    for (int i = 0; i < NEOPIXEL_LED_NUM; i++) {
        LED_data[i][ORDER_R] = (uint8_t)r;
        LED_data[i][ORDER_G] = (uint8_t)g;
        LED_data[i][ORDER_B] = (uint8_t)b;
    }
    SetNeoPixel();
}
