# lpf1 模块用法

头文件：`elib_fl_lpf1.h`（通过 `elib_fl.h` 自动引入）

一阶低通滤波器，零动态分配，支持 float 和 Q32 两种版本，通过 `_Generic` 宏统一 API。

滤波公式：`y[n] = y[n-1] + α × (x[n] - y[n-1])`

---

## Float 版本

```c
#include "elib_fl.h"

elib_fl_lpf1_ctx_f_t ctx;

/* 初始化三选一 */
elib_fl_lpf1_init(&ctx, 0.1f);              /* 直接指定 α */
elib_fl_lpf1_init_tau_f(&ctx, 0.5f, 0.01f); /* 时间常数 */
elib_fl_lpf1_init_fc_f(&ctx, 10.0f, 100.0f);/* 截止频率 */

/* 流式处理 */
float out = elib_fl_lpf1_update(&ctx, raw);

/* 重置 */
elib_fl_lpf1_reset(&ctx);
```

## Q32 版本

```c
#include "elib_fl.h"

elib_fl_lpf1_ctx_q32_t ctx;

/* 初始化三选一，n 为小数位数 */
elib_fl_lpf1_init(&ctx, 6554, 16);               /* α = 0.1 in Q16 */
elib_fl_lpf1_init_tau_q32(&ctx, 0.5f, 0.01f, 16); /* 时间常数 */
elib_fl_lpf1_init_fc_q32(&ctx, 10.0f, 100.0f, 16);/* 截止频率 */

/* 流式处理 — 同名宏，编译器自动选择 Q32 版本 */
int32_t out = elib_fl_lpf1_update(&ctx, raw);

/* 重置 */
elib_fl_lpf1_reset(&ctx);
```

---

## Generic API

使用 `_Generic` 宏，根据 ctx 类型自动分发：

| 宏 | 分发依据 |
|----|---------|
| `elib_fl_lpf1_init(ctx, ...)` | ctx 类型 |
| `elib_fl_lpf1_update(ctx, in)` | ctx 类型 |
| `elib_fl_lpf1_reset(ctx)` | ctx 类型 |

`init_tau` 和 `init_fc` 因参数列表不同，需显式调用 `_f` 或 `_q32` 后缀版本。

---

## 参数选择指南

| α (float) | α (Q16) | 效果 |
|-----------|---------|------|
| 0.01 | 655 | 非常平滑，响应极慢 |
| 0.1 | 6554 | 较平滑，通用场景 |
| 0.5 | 32768 | 平衡响应与平滑 |
| 0.9 | 58982 | 接近直通，响应极快 |

Q32 的 `n` 建议使用 16，兼顾精度和范围。`alpha = (int)(float_alpha × (1 << n) + 0.5f)`

> Q32 限制：输入值范围为 int32_t 全范围。`alpha × diff` 内部使用 int64_t 中间结果后右移回 int32_t，无溢出风险。
