# lpf1 模块用法

头文件：`elib_fl.h`

一阶低通滤波器，零动态分配，纯 C99，提供 float / i16 / i32 / u16 / u32 五套显式接口。

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

## i16 版本

`alpha` 为 Qn 定点数，`n` 为小数位数，取值 1..15。

```c
elib_fl_lpf1_ctx_i16_t ctx;

/* 初始化三选一 */
elib_fl_lpf1_init_i16(&ctx, 3277, 15);                /* α = 0.1 in Q15 */
elib_fl_lpf1_init_tau_i16(&ctx, 0.5f, 0.01f, 15);
elib_fl_lpf1_init_fc_i16(&ctx, 10.0f, 100.0f, 15);

int16_t out = elib_fl_lpf1_update_i16(&ctx, raw);
elib_fl_lpf1_reset_i16(&ctx);
```

## i32 版本

`alpha` 为 Qn 定点数，`n` 为小数位数（建议 16），取值 1..31。

```c
elib_fl_lpf1_ctx_i32_t ctx;

/* 初始化三选一 */
elib_fl_lpf1_init_i32(&ctx, 6554, 16);                /* α = 0.1 in Q16 */
elib_fl_lpf1_init_tau_i32(&ctx, 0.5f, 0.01f, 16);
elib_fl_lpf1_init_fc_i32(&ctx, 10.0f, 100.0f, 16);

int32_t out = elib_fl_lpf1_update_i32(&ctx, raw);
elib_fl_lpf1_reset_i32(&ctx);
```

## u16 版本

```c
elib_fl_lpf1_ctx_u16_t ctx;

elib_fl_lpf1_init_u16(&ctx, 3277, 15);
elib_fl_lpf1_init_tau_u16(&ctx, 0.5f, 0.01f, 15);
elib_fl_lpf1_init_fc_u16(&ctx, 10.0f, 100.0f, 15);

uint16_t out = elib_fl_lpf1_update_u16(&ctx, raw);
elib_fl_lpf1_reset_u16(&ctx);
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
| i16 | `..._init_i16` | `..._init_tau_i16` | `..._init_fc_i16` | `..._update_i16` | `..._reset_i16` |
| i32 | `..._init_i32` | `..._init_tau_i32` | `..._init_fc_i32` | `..._update_i32` | `..._reset_i32` |
| u16 | `..._init_u16` | `..._init_tau_u16` | `..._init_fc_u16` | `..._update_u16` | `..._reset_u16` |
| u32 | `..._init_u32` | `..._init_tau_u32` | `..._init_fc_u32` | `..._update_u32` | `..._reset_u32` |

- `init(ctx, alpha[, n])`：直接指定 α；float 版 α ∈ (0, 1)，整数版 α 为 Qn 定点，`0 < α < 2^n`（α ≥ 2^n 等价于直通，init 拒收）。
- `init_tau(ctx, tau, dt[, n])`：由时间常数换算 α。
- `init_fc(ctx, fc, fs[, n])`：由截止频率换算 α。
- `update(ctx, in)`：处理一个样本，返回滤波输出。
- `reset(ctx)`：清零内部状态。

---

## 参数选择指南

| α (float) | α (Q15) | α (Q16) | 效果 |
|-----------|---------|---------|------|
| 0.01 | 328 | 655 | 非常平滑，响应极慢 |
| 0.1 | 3277 | 6554 | 较平滑，通用场景 |
| 0.5 | 16384 | 32768 | 平衡响应与平滑 |

i32/u32 建议 `n = 16`，i16/u16 建议 `n = 15`：`alpha = (int)(float_alpha × (1 << n) + 0.5f)`（结果须落在 `1 .. 2^n - 1`，否则 init 拒收）。

> v2 舍入提示：换算结果被截断量化到 Qn 时，若四舍五入后恰好等于 `2^n`（≈直通），init 一律拒收——例如 `init_tau(ctx, 1.5e-7f, 1.0f, 16)`。
>
> 32 位版（i32/u32）内部用 int64 中间量计算 `alpha × (in - out)`，in/out 取全 32 位范围也不溢出；16 位版（i16/u16）用 int32，`|α × (in − out)| ≤ 32767 × 65535 < 2^31`，同样恒不溢出。
