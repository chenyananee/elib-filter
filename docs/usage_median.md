# median 模块用法

头文件：`elib_fl_median.h`（通过 `elib_fl.h` 自动引入）

中位值滤波器，零动态分配，非重叠窗口，支持 float 和 Q32 两种版本。

---

## Float 版本

```c
#include "elib_fl.h"

float buf[5];
elib_fl_median_ctx_f_t ctx;
elib_fl_median_init(&ctx, buf, 5);

while (1) {
    float out = elib_fl_median_update(&ctx, read_sensor());
    if (ctx.count == 0) {
        use_output(out);  /* 窗口满，有效中位值 */
    }
}

elib_fl_median_reset(&ctx);
```

## Q32 版本

```c
int32_t buf[5];
elib_fl_median_ctx_q32_t ctx;
elib_fl_median_init(&ctx, buf, 5);

while (1) {
    int32_t out = elib_fl_median_update(&ctx, read_adc());
    if (ctx.count == 0) {
        use_output(out);
    }
}
```

---

## Generic API

| 宏 | 分发依据 |
|----|---------|
| `elib_fl_median_init(ctx, ...)` | ctx 类型 |
| `elib_fl_median_update(ctx, in)` | ctx 类型 |
| `elib_fl_median_reset(ctx)` | ctx 类型 |

---

## 参数选择指南

| 窗口大小 | 效果 |
|----------|------|
| 3 | 轻度过滤，响应快 |
| 5 | 通用场景，推荐 |
| 7 | 较强过滤，延迟增大 |
| 9+ | 强过滤，适用于高脉冲噪声 |

适用于：消除椒盐噪声、传感器脉冲干扰、ADC 毛刺。
