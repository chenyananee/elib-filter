# avg 模块用法

头文件：`elib_fl_avg.h`（通过 `elib_fl.h` 自动引入）

平均滤波器，零动态分配，非重叠窗口，纯 C99，提供 float / i32 / u32 三套显式接口。

---

## Float 版本

```c
#include "elib_fl.h"

elib_fl_avg_ctx_f_t ctx;
elib_fl_avg_init_f(&ctx, 8);

while (1) {
    float out = elib_fl_avg_update_f(&ctx, read_sensor());
    if (elib_fl_avg_warmup_f(&ctx)) {
        use_output(out);  /* 本次凑满窗口，out 为有效均值 */
    }
}

elib_fl_avg_reset_f(&ctx);
```

## i32 版本

```c
elib_fl_avg_ctx_i32_t ctx;
elib_fl_avg_init_i32(&ctx, 8);

while (1) {
    int32_t out = elib_fl_avg_update_i32(&ctx, read_adc());
    if (elib_fl_avg_warmup_i32(&ctx)) {
        use_output(out);
    }
}
```

## u32 版本

```c
elib_fl_avg_ctx_u32_t ctx;
elib_fl_avg_init_u32(&ctx, 8);

while (1) {
    uint32_t out = elib_fl_avg_update_u32(&ctx, read_counter());
    if (elib_fl_avg_warmup_u32(&ctx)) {
        use_output(out);
    }
}
```

---

## API

| 版本 | init | update | warmup | reset |
|------|------|--------|--------|-------|
| float | `elib_fl_avg_init_f` | `elib_fl_avg_update_f` | `elib_fl_avg_warmup_f` | `elib_fl_avg_reset_f` |
| i32 | `elib_fl_avg_init_i32` | `elib_fl_avg_update_i32` | `elib_fl_avg_warmup_i32` | `elib_fl_avg_reset_i32` |
| u32 | `elib_fl_avg_init_u32` | `elib_fl_avg_update_u32` | `elib_fl_avg_warmup_u32` | `elib_fl_avg_reset_u32` |

- `init(ctx, size)`：设置窗口大小（必须 > 0），返回 `ELIB_FL_OK` 或 `ELIB_FL_ERR_INVALID_PARAM`。
- `update(ctx, in)`：累加样本；窗口未满返回 0，满则返回均值并清空窗口。
- `warmup(ctx)`：就绪标志。仅当最近一次 `update` 凑满窗口、返回值为有效均值时为 `1`，否则为 `0`。
- `reset(ctx)`：清空累加器与计数。

> `update` 在窗口未满时返回 0，而 0 也可能是合法均值，故不能靠返回值判断有效性；用 `warmup(ctx) == 1` 判断本次输出是否有效。

> 溢出边界：`sum` 与样本同宽。i32 要求窗口内 `|样本|` 之和 ≤ `INT32_MAX`；u32 要求窗口内样本之和 ≤ `UINT32_MAX`。

---

## 参数选择指南

| 窗口大小 | 效果 |
|----------|------|
| 4 | 轻度平滑 |
| 8 | 通用场景，推荐 |
| 16 | 较强平滑 |
| 32+ | 强平滑，适用于慢变信号 |

适用于：ADC 采样平滑、稳态信号降噪。
