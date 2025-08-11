#include "push_sw.h"

#define SW_HISTORY_LEN 10
#define MAX_SWITCHES  10  // 登録できる最大のスイッチ数（必要に応じて調整）

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    uint8_t prev_state;
} SwitchState;

static SwitchState switch_states[MAX_SWITCHES];
static int switch_count = 0;

/**
 * @brief 内部テーブルから該当スイッチの状態を取得（なければ新規登録）
 */
static SwitchState* get_switch_state(GPIO_TypeDef* port, uint16_t pin) {
    for (int i = 0; i < switch_count; ++i) {
        if (switch_states[i].port == port && switch_states[i].pin == pin) {
            return &switch_states[i];
        }
    }

    // 新規登録
    if (switch_count < MAX_SWITCHES) {
        switch_states[switch_count].port = port;
        switch_states[switch_count].pin = pin;
        switch_states[switch_count].prev_state = 0;
        return &switch_states[switch_count++];
    }

    return NULL;  // 登録上限
}

/**
 * @brief スイッチが「押された瞬間」だけ1を返す（チャタリング対策付き）
 */
uint16_t readPushSwitch(GPIO_TypeDef* port, uint16_t pin) {
    SwitchState* sw = get_switch_state(port, pin);
    if (!sw) return 0;  // 取得失敗

    // チャタリング対策：10回連続でLOWなら押下判定
    for (int i = 0; i < SW_HISTORY_LEN; ++i) {
        if (HAL_GPIO_ReadPin(port, pin) != GPIO_PIN_RESET) {
            sw->prev_state = 0;
            return 0;  // 離されている
        }
        HAL_Delay(1);
    }

    // 前回が離されていて、今回押されていたら「押された瞬間」
    if (sw->prev_state == 0) {
        sw->prev_state = 1;
        return 1;
    }

    return 0;  // 押しっぱなし中
}
