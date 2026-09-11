# lpf1 模块用法

头文件：`elib_fl_lpf1.h`（通过 `elib_fl.h` 自动引入）

一阶低通滤波器，零动态分配，纯 C99，提供 float / i32 / u32 三套显式接口。

滤波公式：`y[n] = y[n-1] + α × (x[n] - y[n-1])`

---

## Float 版本

```c
#include "elib_fl.h"

elib_fl_lpf1_ctx_f_t ctx;

/* 初始化三选一 */
elib_fl_lpf1_init_f(&ctx, 0.1f);              /* 直接指定 α */
elib_fl_lpf1_init_tau_f(&ctx, 0.5f, 0.01f);   /* 时间常数 */
elib_fl_lpf1_init_fc_f(&ctx, 10.0f, 100.0f);  /* 截止频率 */

float out = elib_fl_lpf1_update_f(&ctx, raw);
elib_fl_lpf1_reset_f(&ctx);
```

## i32 版本

`alpha` 为 Qn 定点数，`n` 为小数位数（建议 16）。

```c
elib_fl_lpf1_ctx_i32_t ctx;

/* 初始化三选一 */
elib_fl_lpf1_init_i32(&ctx, 6554, 16);               /* α = 0.1 in Q16 */
elib_fl_lpf1_init_tau_i32(&ctx, 0.5f, 0.01f, 16);
elib_fl_lpf1_init_fc_i32(&ctx, 10.0f, 100.0f, 16);

int32_t out = elib_fl_lpf1_update_i32(&ctx, raw);
elib_fl_lpf1_reset_i32(&ctx);
```

## u32 版本

```c
elib_fl_lpf1_ctx_u32_t ctx;

elib_fl_lpf1_init_u32(&ctx, 6554, 16);
elib_fl_lpf1_init_tau_u32(&ctx, 0.5f, 0.01f, 16);
elib_fl_lpf1_init_fc_u32(&ctx, 10.0f, 100.0f, 16);

uint32_t out = elib_fl_lpf1_update_u32(&ctx, raw);
elib_fl_lpf1_reset_u32(&ctx);
```

---

## API

| 版本 | init | init_tau | init_fc | update | reset |
|------|------|----------|---------|--------|-------|
| float | `..._init_f` | `..._init_tau_f` | `..._init_fc_f` | `..._update_f` | `..._reset_f` |
| i32 | `..._init_i32` | `..._init_tau_i32` | `..._init_fc_i32` | `..._update_i32` | `..._reset_i32` |
| u32 | `..._init_u32` | `..._init_tau_u32` | `..._init_fc_u32` | `..._update_u32` | `..._reset_u32` |

- `init(ctx, alpha[, n])`：直接指定 α；float 版 α ∈ (0, 1)，i32/u32 版 α 为 Qn 定点，`0 < α < 2^n`。
- `init_tau(ctx, tau, dt[, n])`：由时间常数换算 α。
- `init_fc(ctx, fc, fs[, n])`：由截止频率换算 α。
- `update(ctx, in)`：处理一个样本，返回滤波输出。
- `reset(ctx)`：清零内部状态。

---

## 参数选择指南

| α (float) | α (Q16) | 效果 |
|-----------|---------|------|
| 0.01 | 655 | 非常平滑，响应极慢 |
| 0.1 | 6554 | 较平滑，通用场景 |
| 0.5 | 32768 | 平衡响应与平滑 |
| 0.9 | 58982 | 接近直通，响应极快 |

i32/u32 建议 `n = 16`，兼顾精度和范围：`alpha = (int)(float_alpha × (1 << n) + 0.5f)`。

> i32/u32 内部用 int64 有符号中间量计算 `alpha × (in - out)`，输入取全 int32 范围也不溢出。
