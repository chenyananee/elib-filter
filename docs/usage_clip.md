# clip 模块用法

头文件：`elib_fl_clip.h`（通过 `elib_fl.h` 自动引入）

限幅滤波器，零动态分配，支持 float 和 Q32 两种版本。

判定规则：`|x[n] - y[n-1]| ≤ threshold → y[n] = x[n]`，否则 `y[n] = y[n-1]`

---

## Float 版本

```c
#include "elib_fl.h"

elib_fl_clip_ctx_f_t ctx;
elib_fl_clip_init(&ctx, 5.0f);         /* threshold = 5.0 */
elib_fl_clip_set_f(&ctx, 100.0f);      /* 设置初始值 */

float out = elib_fl_clip_update(&ctx, raw);
elib_fl_clip_reset(&ctx);
```

## Q32 版本

```c
elib_fl_clip_ctx_q32_t ctx;
elib_fl_clip_init(&ctx, 500, 8);       /* threshold = 500 in Q8 */
elib_fl_clip_set_q32(&ctx, 10000);     /* 设置初始值 */

int32_t out = elib_fl_clip_update(&ctx, raw);
elib_fl_clip_reset(&ctx);
```

---

## Generic API

| 宏 | 分发依据 |
|----|---------|
| `elib_fl_clip_init(ctx, ...)` | ctx 类型 |
| `elib_fl_clip_set(ctx, ...)` | ctx 类型 |
| `elib_fl_clip_update(ctx, in)` | ctx 类型 |
| `elib_fl_clip_reset(ctx)` | ctx 类型 |

---

## 参数选择指南

| threshold | 效果 |
|-----------|------|
| 小值 | 仅抑制大幅跳变，对噪声敏感 |
| 中值 | 通用场景，平衡响应与抗干扰 |
| 大值 | 仅过滤极端异常值 |

适用于：消除传感器毛刺、阶跃干扰等突发异常值。

> Q32 限制：`|in - prev|` 使用 int32_t 差值计算，当 in 和 prev 符号相反且差值超过 INT32_MAX 时会溢出。实际使用中 in 和 prev 通常同量级，无此风险。
