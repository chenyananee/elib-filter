# deadband 模块用法

头文件：`elib_fl.h`

死区滤波器，零动态分配，纯 C99，提供 float / i16 / i32 / u16 / u32 五套显式接口。

判定规则：

```
|x[n] - y[n-1]| ≤ threshold → y[n] = y[n-1]   （保持，忽略小变化）
|x[n] - y[n-1]| >  threshold → y[n] = x[n]    （大变化跟随）
```

与 clip 互为镜像：clip 吞**大**突变（抗尖刺），deadband 吞**小**变化（抗抖动）。死区含边界（`≤` 保持，`>` 才跟随）。

---

## Float 版本

```c
#include "elib_fl.h"

elib_fl_deadband_ctx_f_t ctx;
elib_fl_deadband_init_f(&ctx, 5.0f);      /* 死区宽度 = 5.0 */
elib_fl_deadband_set_f(&ctx, 100.0f);     /* 设置初始参考值 */

float out = elib_fl_deadband_update_f(&ctx, raw);
elib_fl_deadband_reset_f(&ctx);
```

## i16 版本

```c
elib_fl_deadband_ctx_i16_t ctx;
elib_fl_deadband_init_i16(&ctx, 50);      /* 死区宽度 = 50 */
elib_fl_deadband_set_i16(&ctx, 1000);

int16_t out = elib_fl_deadband_update_i16(&ctx, raw);
elib_fl_deadband_reset_i16(&ctx);
```

## i32 版本

```c
elib_fl_deadband_ctx_i32_t ctx;
elib_fl_deadband_init_i32(&ctx, 500);
elib_fl_deadband_set_i32(&ctx, 10000);

int32_t out = elib_fl_deadband_update_i32(&ctx, raw);
elib_fl_deadband_reset_i32(&ctx);
```

## u16 版本

```c
elib_fl_deadband_ctx_u16_t ctx;
elib_fl_deadband_init_u16(&ctx, 50);
elib_fl_deadband_set_u16(&ctx, 1000);

uint16_t out = elib_fl_deadband_update_u16(&ctx, raw);
elib_fl_deadband_reset_u16(&ctx);
```

## u32 版本

```c
elib_fl_deadband_ctx_u32_t ctx;
elib_fl_deadband_init_u32(&ctx, 500);
elib_fl_deadband_set_u32(&ctx, 10000);

uint32_t out = elib_fl_deadband_update_u32(&ctx, raw);
elib_fl_deadband_reset_u32(&ctx);
```

---

## API

| 版本 | init | set | update | reset |
|------|------|-----|--------|-------|
| float | `elib_fl_deadband_init_f` | `elib_fl_deadband_set_f` | `elib_fl_deadband_update_f` | `elib_fl_deadband_reset_f` |
| i16 | `elib_fl_deadband_init_i16` | `elib_fl_deadband_set_i16` | `elib_fl_deadband_update_i16` | `elib_fl_deadband_reset_i16` |
| i32 | `elib_fl_deadband_init_i32` | `elib_fl_deadband_set_i32` | `elib_fl_deadband_update_i32` | `elib_fl_deadband_reset_i32` |
| u16 | `elib_fl_deadband_init_u16` | `elib_fl_deadband_set_u16` | `elib_fl_deadband_update_u16` | `elib_fl_deadband_reset_u16` |
| u32 | `elib_fl_deadband_init_u32` | `elib_fl_deadband_set_u32` | `elib_fl_deadband_update_u32` | `elib_fl_deadband_reset_u32` |

- `init(ctx, threshold)`：设置死区宽度（必须 > 0；0 等价于每次变化都跟随的直通，init 拒收）。
- `set(ctx, value)`：设置初始输出值（死区以它为参考起点），返回 `ELIB_FL_OK` 或 `ELIB_FL_ERR_NOT_INITIALIZED`。
- `update(ctx, in)`：与当前输出比较，差值 ≤ 死区宽度则保持输出，超出则跟随输入。
- `reset(ctx)`：清零输出；阈值保留。参考起点回到 0，死区围绕 0 重新生效。

无窗口、无 warmup：首个 update 即有效输出，无 ready 接口。

---

## 参数选择指南

| threshold | 效果 |
|-----------|------|
| 接近 0 | 几乎不过滤，仅吞完全相等的重复值 |
| ≈ 噪声峰峰值 / 2 | **推荐**：恰好凝住噪声往返，真变化能穿透 |
| 大值 | 输出近乎冻结，仅极端值才跟随 |

- 静态显示（万用表/面板读数）取 1–3 个 LSB 的死区即可凝住末位抖动。
- 变化上报/日志场景：死区宽度即"最小可报告变化量"。
- 继电器/阀门控制：死区宽度即"最小不动作带"，防执行器磨损。

## 典型链路

```
median（去毛刺） → lpf1/mavg（平滑） → deadband（凝固输出）
```

---

> i16/u16 内部用 int32 计算 `in - out` 差值（|diff| ≤ 2^16−1，恒不溢出）；i32/u32 内部用 int64，in/out 取全 32 位范围也不溢出。
