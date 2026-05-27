# avg 模块用法

头文件：`elib_fl_avg.h`（通过 `elib_fl.h` 自动引入）

平均滤波器，零动态分配，非重叠窗口，支持 float 和 Q32 两种版本。

---

## Float 版本

```c
#include "elib_fl.h"

elib_fl_avg_ctx_f_t ctx;
elib_fl_avg_init(&ctx, 8);

while (1) {
    float out = elib_fl_avg_update(&ctx, read_sensor());
    if (ctx.count == 0) {
        use_output(out);  /* 窗口满，有效均值 */
    }
}

elib_fl_avg_reset(&ctx);
```

## Q32 版本

```c
elib_fl_avg_ctx_q32_t ctx;
elib_fl_avg_init(&ctx, 8);

while (1) {
    int32_t out = elib_fl_avg_update(&ctx, read_adc());
    if (ctx.count == 0) {
        use_output(out);
    }
}
```

> Q32 版本使用 int32_t 累加器，注意溢出限制：`累加和 ≤ INT32_MAX`，即单个采样值 × 窗口大小不超过 2,147,483,647。
> 例：窗口 16，采样值上限 = 2,147,483,647 / 16 = 134,217,727。

---

## Generic API

| 宏 | 分发依据 |
|----|---------|
| `elib_fl_avg_init(ctx, ...)` | ctx 类型 |
| `elib_fl_avg_update(ctx, in)` | ctx 类型 |
| `elib_fl_avg_reset(ctx)` | ctx 类型 |

---

## 参数选择指南

| 窗口大小 | 效果 |
|----------|------|
| 4 | 轻度平滑 |
| 8 | 通用场景，推荐 |
| 16 | 较强平滑 |
| 32+ | 强平滑，适用于慢变信号 |

适用于：ADC 采样平滑、稳态信号降噪。
