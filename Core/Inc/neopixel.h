#ifndef BASICFUNCTIONS_INC_NEOPIXEL_H_
#define BASICFUNCTIONS_INC_NEOPIXEL_H_

#include "tim.h"
#include <stdint.h>

/**
 * @brief NeoPixel ドライバ初期化（任意）
 *
 * - コンパイル時フラグ NEOPIXEL_USE_AUTOTIMING が 1 のとき:
 *   タイマ ARR から 1bit 内部 tick 数を再計算してセット
 * - 0 のとき: 何もしません（固定定数で動作）
 *
 * CubeMX の初期化後（MX_TIM1_Init() の直後）に呼んでください。
 * 呼ばなくても動きます（既定は固定定数）。
 */
void NeoPixel_Init(void);

/**
 * @brief すべての LED を同一色に設定して送信（GRB順で出力）
 * @param r 0..255
 * @param g 0..255
 * @param b 0..255
 */
void UpdateNeoPixel(uint16_t r, uint16_t g, uint16_t b);

/**
 * @brief LED_data に積んだ内容を送信（全LED一括）
 * - 今回は内部で UpdateNeoPixel() からも呼ばれる想定
 */
void SetNeoPixel(void);

#endif /* BASICFUNCTIONS_INC_NEOPIXEL_H_ */
