# slew 模块用法

头文件：`elib_fl.h`

压摆率限幅器（slew-rate limiter，v2 及之前叫 `clip`，v3 起正名），零动态分配，纯 C99，提供 float / i16 / i32 / u16 / u32 五套显式接口。

判定规则：`|x[n] - y[n-1]| ≤ threshold → y[n] = x[n]`， 否则 `y[n] = y[n-1]（步进超出阈值即拒绝，保持上次输出）`

**不是幅度钳制**：每个输出步的斜率受限，输出是"变化率被限住"的信号，不是上下限包络。

与 deadband 互为镜像：slew 吞**大**突变（抗尖刺），deadband 吞**小**变化（抗抖动）。

---

## Float 版本

```c
#include "elib_fl.h"

elib_fl_slew_ctx_f_t ctx;
elib_fl_slew_init_f(&ctx, 5.0f);      /* threshold = 5.0 */
elib_fl_slew_set_f(&ctx, 100.0f);     /* 设置初始值 */

float out = elib_fl_slew_update_f(&ctx, raw);
elib_fl_slew_reset_f(&ctx);
```

## i16 版本

```c
elib_fl_slew_ctx_i16_t ctx;
elib_fl_slew_init_i16(&ctx, 50);      /* threshold = 50 */
elib_fl_slew_set_i16(&ctx, 1000);

int16_t out = elib_fl_slew_update_i16(&ctx, raw);
elib_fl_slew_reset_i16(&ctx);
```

## i32 版本

```c
elib_fl_slew_ctx_i32_t ctx;
elib_fl_slew_init_i32(&ctx, 500);
elib_fl_slew_set_i32(&ctx, 10000);

int32_t out = elib_fl_slew_update_i32(&ctx, raw);
elib_fl_slew_reset_i32(&ctx);
```

## u16 版本

```c
elib_fl_slew_ctx_u16_t ctx;
elib_fl_slew_init_u16(&ctx, 50);
elib_fl_slew_set_u16(&ctx, 1000);

uint16_t out = elib_fl_slew_update_u16(&ctx, raw);
elib_fl_slew_reset_u16(&ctx);
```

## u32 版本

```c
elib_fl_slew_ctx_u32_t ctx;
elib_fl_slew_init_u32(&ctx, 500);
elib_fl_slew_set_u32(&ctx, 10000);

uint32_t out = elib_fl_slew_update_u32(&ctx, raw);
elib_fl_slew_reset_u32(&ctx);
```

---

## API

| 版本 | init | set | update | reset |
|------|------|-----|--------|-------|
| float | `elib_fl_slew_init_f` | `elib_fl_slew_set_f` | `elib_fl_slew_update_f` | `elib_fl_slew_reset_f` |
| i16 | `elib_fl_slew_init_i16` | `elib_fl_slew_set_i16` | `elib_fl_slew_update_i16` | `elib_fl_slew_reset_i16` |
| i32 | `elib_fl_slew_init_i32` | `elib_fl_slew_set_i32` | `elib_fl_slew_update_i32` | `elib_fl_slew_reset_i32` |
| u16 | `elib_fl_slew_init_u16` | `elib_fl_slew_set_u16` | `elib_fl_slew_update_u16` | `elib_fl_slew_reset_u16` |
| u32 | `elib_fl_slew_init_u32` | `elib_fl_slew_set_u32` | `elib_fl_slew_update_u32` | `elib_fl_slew_reset_u32` |

- `init(ctx, threshold)`：设置限幅阈值（必须 > 0）。
- `set(ctx, value)`：设置初始输出值，返回 `ELIB_FL_OK` 或 `ELIB_FL_ERR_NOT_INITIALIZED`。
- `update(ctx, in)`：与当前输出比较，差值在阈值内则接受新值，否则保持。
- `reset(ctx)`：清零输出。

> **v2 → v3 迁移**：`elib_fl_clip_*` 全部改名 `elib_fl_slew_*`（类型 10 个、函数 20 个），语义与参数完全不变；头文件 `elib_fl_clip.h` 早在 v2 已并入 `elib_fl.h`，无其它迁移成本。

---

## 参数选择指南

| threshold | 效果 |
|-----------|------|
| 小值 | 仅抑制大幅跳变，对噪声敏感 |
| 中值 | 通用场景，平衡响应与抗干扰 |
| 大值 | 仅过滤极端异常值 |

适用于：消除传感器毛刺、阶跃干扰等突发异常值。

> i32/u32 内部用 int64 计算 `in - out` 的绝对值，in/out 取全 32 位范围也不溢出；i16/u16 内部用 int32（|diff| ≤ 2^16−1），同样恒不溢出。
