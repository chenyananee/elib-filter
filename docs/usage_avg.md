# avg 模块用法

头文件：`elib_fl.h`

平均滤波器，零动态分配，非重叠窗口（每 `size` 个样本出一拍后重启），纯 C99，提供 float / i16 / i32 / u16 / u32 五套显式接口。

---

## Float 版本

```c
#include "elib_fl.h"

elib_fl_avg_ctx_f_t ctx;
elib_fl_avg_init_f(&ctx, 8);

while (1) {
    float out = elib_fl_avg_update_f(&ctx, read_sensor());
    if (elib_fl_avg_ready_f(&ctx)) {
        use_output(out);  /* 本次凑满窗口，out 为有效均值 */
    }
}

elib_fl_avg_reset_f(&ctx);
```

## i16 版本

```c
elib_fl_avg_ctx_i16_t ctx;
elib_fl_avg_init_i16(&ctx, 8);   /* size ≤ 65535，超限 init 拒收 */

while (1) {
    int16_t out = elib_fl_avg_update_i16(&ctx, read_adc_i16());
    if (elib_fl_avg_ready_i16(&ctx)) {
        use_output(out);
    }
}
```

## i32 版本

```c
elib_fl_avg_ctx_i32_t ctx;
elib_fl_avg_init_i32(&ctx, 8);   /* 64 位累加器，任意窗长 */

while (1) {
    int32_t out = elib_fl_avg_update_i32(&ctx, read_adc());
    if (elib_fl_avg_ready_i32(&ctx)) {
        use_output(out);
    }
}
```

## u16 版本

```c
elib_fl_avg_ctx_u16_t ctx;
elib_fl_avg_init_u16(&ctx, 8);   /* size ≤ 65535，超限 init 拒收 */

while (1) {
    uint16_t out = elib_fl_avg_update_u16(&ctx, read_counter16());
    if (elib_fl_avg_ready_u16(&ctx)) {
        use_output(out);
    }
}
```

## u32 版本

```c
elib_fl_avg_ctx_u32_t ctx;
elib_fl_avg_init_u32(&ctx, 8);   /* 64 位累加器，任意窗长 */

while (1) {
    uint32_t out = elib_fl_avg_update_u32(&ctx, read_counter());
    if (elib_fl_avg_ready_u32(&ctx)) {
        use_output(out);
    }
}
```

---

## API

| 版本 | init | update | ready | reset |
|------|------|--------|-------|-------|
| float | `elib_fl_avg_init_f` | `elib_fl_avg_update_f` | `elib_fl_avg_ready_f` | `elib_fl_avg_reset_f` |
| i16 | `elib_fl_avg_init_i16` | `elib_fl_avg_update_i16` | `elib_fl_avg_ready_i16` | `elib_fl_avg_reset_i16` |
| i32 | `elib_fl_avg_init_i32` | `elib_fl_avg_update_i32` | `elib_fl_avg_ready_i32` | `elib_fl_avg_reset_i32` |
| u16 | `elib_fl_avg_init_u16` | `elib_fl_avg_update_u16` | `elib_fl_avg_ready_u16` | `elib_fl_avg_reset_u16` |
| u32 | `elib_fl_avg_init_u32` | `elib_fl_avg_update_u32` | `elib_fl_avg_ready_u32` | `elib_fl_avg_reset_u32` |

- `init(ctx, size)`：设置窗口大小（必须 > 0；i16/u16 版另须 ≤ 65535），返回 `ELIB_FL_OK` 或 `ELIB_FL_ERR_INVALID_PARAM`。
- `update(ctx, in)`：累加样本；窗口未满返回 0（哨兵），满则返回均值（整数版向零截断）并清空窗口。
- `ready(ctx)`：就绪标志。仅当最近一次 `update` 凑满窗口、返回值为有效均值时为 `1`，否则为 `0`（v1 的 `warmup_*` 在 v2 改名为 `ready_*`，0/1 语义不变）。
- `reset(ctx)`：清空累加器与计数。

> `update` 在窗口未满时返回 0，而 0 也可能是合法均值，故不能靠返回值判断有效性；用 `ready(ctx) == 1` 判断本次输出是否有效。

> **溢出边界（v2）**：i32/u32 版累加器为 64 位，**任意样本值 × 任意窗长**均合法，无需用户计算界（v1 的 32 位累加器 UB/回绕已消除）。i16/u16 版累加器为 32 位，最坏窗和以 init 强制 `size ≤ 65535` 保证恒不溢出。

---

## 参数选择指南

| 窗口大小 | 效果 |
|----------|------|
| 4 | 轻度平滑 |
| 8 | 通用场景，推荐 |
| 16 | 较强平滑 |
| 32+ | 强平滑，适用于慢变信号 |

适用于：ADC 采样平滑、稳态信号降噪。
