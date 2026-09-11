# clip 模块用法

头文件：`elib_fl_clip.h`（通过 `elib_fl.h` 自动引入）

限幅滤波器，零动态分配，纯 C99，提供 float / i32 / u32 三套显式接口。

判定规则：`|x[n] - y[n-1]| ≤ threshold → y[n] = x[n]`，否则 `y[n] = y[n-1]`

---

## Float 版本

```c
#include "elib_fl.h"

elib_fl_clip_ctx_f_t ctx;
elib_fl_clip_init_f(&ctx, 5.0f);      /* threshold = 5.0 */
elib_fl_clip_set_f(&ctx, 100.0f);     /* 设置初始值 */

float out = elib_fl_clip_update_f(&ctx, raw);
elib_fl_clip_reset_f(&ctx);
```

## i32 版本

```c
elib_fl_clip_ctx_i32_t ctx;
elib_fl_clip_init_i32(&ctx, 500);     /* threshold = 500 */
elib_fl_clip_set_i32(&ctx, 10000);    /* 设置初始值 */

int32_t out = elib_fl_clip_update_i32(&ctx, raw);
elib_fl_clip_reset_i32(&ctx);
```

## u32 版本

```c
elib_fl_clip_ctx_u32_t ctx;
elib_fl_clip_init_u32(&ctx, 500);
elib_fl_clip_set_u32(&ctx, 10000);

uint32_t out = elib_fl_clip_update_u32(&ctx, raw);
elib_fl_clip_reset_u32(&ctx);
```

---

## API

| 版本 | init | set | update | reset |
|------|------|-----|--------|-------|
| float | `elib_fl_clip_init_f` | `elib_fl_clip_set_f` | `elib_fl_clip_update_f` | `elib_fl_clip_reset_f` |
| i32 | `elib_fl_clip_init_i32` | `elib_fl_clip_set_i32` | `elib_fl_clip_update_i32` | `elib_fl_clip_reset_i32` |
| u32 | `elib_fl_clip_init_u32` | `elib_fl_clip_set_u32` | `elib_fl_clip_update_u32` | `elib_fl_clip_reset_u32` |

- `init(ctx, threshold)`：设置限幅阈值（必须 > 0）。
- `set(ctx, value)`：设置初始输出值，返回 `ELIB_FL_OK` 或 `ELIB_FL_ERR_NOT_INITIALIZED`。
- `update(ctx, in)`：与当前输出比较，差值在阈值内则接受新值，否则保持。
- `reset(ctx)`：清零输出。

---

## 参数选择指南

| threshold | 效果 |
|-----------|------|
| 小值 | 仅抑制大幅跳变，对噪声敏感 |
| 中值 | 通用场景，平衡响应与抗干扰 |
| 大值 | 仅过滤极端异常值 |

适用于：消除传感器毛刺、阶跃干扰等突发异常值。

> i32/u32 内部用 int64 计算 `in - out` 的绝对值，in/out 取全 32 位范围也不溢出。
