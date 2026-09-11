# elib-filter

零动态内存分配的嵌入式 C 滤波算法库，纯 C99，无外部依赖。每个滤波器提供 float / i32 / u32 三套实现，显式后缀调用，不使用宏分发。

## 用法文档

| 模块 | 文档 |
|------|------|
| lpf1 | [docs/usage_lpf1.md](docs/usage_lpf1.md) |
| clip | [docs/usage_clip.md](docs/usage_clip.md) |
| median | [docs/usage_median.md](docs/usage_median.md) |
| avg | [docs/usage_avg.md](docs/usage_avg.md) |
| mavg | [docs/usage_mavg.md](docs/usage_mavg.md) |

## 模块列表

| 模块 | 头文件 | 说明 |
|------|--------|------|
| lpf1 | `elib_fl_lpf1.h` | 一阶低通滤波器 |
| clip | `elib_fl_clip.h` | 限幅滤波器 |
| median | `elib_fl_median.h` | 中位值滤波器 |
| avg | `elib_fl_avg.h` | 平均滤波器 |
| mavg | `elib_fl_mavg.h` | 滑动平均滤波器 |
| err | `elib_fl_err.h` | 统一错误码 |

用户只需 `#include "elib_fl.h"` 即可引入全部模块，也可单独引用子模块头文件。

## 三版本 API

每个滤波器提供 **float** / **i32** / **u32** 三套实现，按类型显式调用对应后缀，不使用 `_Generic` 等宏分发，纯 C99 可编译。

| 后缀 | 上下文类型 | 说明 |
|------|-----------|------|
| `_f` | `elib_fl_xxx_ctx_f_t` | 单精度浮点 |
| `_i32` | `elib_fl_xxx_ctx_i32_t` | 有符号 32 位 |
| `_u32` | `elib_fl_xxx_ctx_u32_t` | 无符号 32 位 |

```c
/* float 版本 */
elib_fl_lpf1_ctx_f_t ctx_f;
elib_fl_lpf1_init_f(&ctx_f, 0.1f);
float out_f = elib_fl_lpf1_update_f(&ctx_f, 3.14f);

/* i32 版本 — alpha 为 Qn 定点数，n 为小数位数 */
elib_fl_lpf1_ctx_i32_t ctx_q;
elib_fl_lpf1_init_i32(&ctx_q, 6554, 16);   /* alpha = 0.1 in Q16 */
int32_t out_q = elib_fl_lpf1_update_i32(&ctx_q, 100);
```

lpf1 的 `_i32` / `_u32` 版本 alpha 为 Qn 定点数，需额外传小数位数 `n`（建议 16）；其余滤波器按各自类型直接传参。

## 功能列表

### lpf1 — 一阶低通滤波器

> 滤波公式：`y[n] = y[n-1] + α × (x[n] - y[n-1])`

| 函数 | float | i32 / u32 |
|------|-------|-----------|
| `elib_fl_lpf1_init_*(ctx, alpha)` | α: (0.0, 1.0) | alpha: Qn 定点, + n |
| `elib_fl_lpf1_init_tau_*(ctx, tau, dt)` | tau/dt: 秒 | 同左 + n |
| `elib_fl_lpf1_init_fc_*(ctx, fc, fs)` | fc/fs: Hz | 同左 + n |
| `elib_fl_lpf1_update_*(ctx, in)` | float → float | int32/uint32 → 同型 |
| `elib_fl_lpf1_reset_*(ctx)` | ✓ | ✓ |

### clip — 限幅滤波器

> `|x[n] - y[n-1]| ≤ threshold → y[n] = x[n]`，否则保持 y[n-1]

| 函数 | float | i32 | u32 |
|------|-------|-----|-----|
| `elib_fl_clip_init_*(ctx, threshold)` | float | int32 | uint32 |
| `elib_fl_clip_set_*(ctx, value)` | float | int32 | uint32 |
| `elib_fl_clip_update_*(ctx, in)` | float → float | int32 → int32 | uint32 → uint32 |
| `elib_fl_clip_reset_*(ctx)` | ✓ | ✓ | ✓ |

### median — 中位值滤波器

> 收集 N 个样本排序取中位值，非重叠窗口

| 函数 | float | i32 | u32 |
|------|-------|-----|-----|
| `elib_fl_median_init_*(ctx, buf, size)` | float* | int32_t* | uint32_t* |
| `elib_fl_median_update_*(ctx, in)` | float → float | int32 → int32 | uint32 → uint32 |
| `elib_fl_median_warmup_*(ctx)` | ✓ | ✓ | ✓ |
| `elib_fl_median_reset_*(ctx)` | ✓ | ✓ | ✓ |

### avg — 平均滤波器

> 收集 N 个样本求均值，非重叠窗口

| 函数 | float | i32 | u32 |
|------|-------|-----|-----|
| `elib_fl_avg_init_*(ctx, size)` | ✓ | ✓ | ✓ |
| `elib_fl_avg_update_*(ctx, in)` | float → float | int32 → int32 | uint32 → uint32 |
| `elib_fl_avg_warmup_*(ctx)` | ✓ | ✓ | ✓ |
| `elib_fl_avg_reset_*(ctx)` | ✓ | ✓ | ✓ |

### mavg — 滑动平均滤波器

> 滑动窗口，O(1) 更新，三版本：float / i32 / u32

| 函数 | float | i32 | u32 |
|------|-------|-----|-----|
| `elib_fl_mavg_init_*(ctx, buf, size)` | float* | int32_t* | uint32_t* |
| `elib_fl_mavg_update_*(ctx, in)` | float → float | int32 → int32 | uint32 → uint32 |
| `elib_fl_mavg_warmup_*(ctx)` | ✓ | ✓ | ✓ |
| `elib_fl_mavg_reset_*(ctx)` | ✓ | ✓ | ✓ |

### 错误码

| 错误码 | 说明 |
|--------|------|
| `ELIB_FL_OK` | 成功 |
| `ELIB_FL_ERR_INVALID_PARAM` | 无效参数 |
| `ELIB_FL_ERR_NOT_INITIALIZED` | 未初始化 |

## 目录结构

```
elib-filter/
├── include/
│   ├── elib_fl.h                  # 伞形头文件
│   ├── elib_fl_err.h              # 错误码
│   ├── elib_fl_lpf1.h             # 一阶低通滤波器
│   ├── elib_fl_clip.h             # 限幅滤波器
│   ├── elib_fl_median.h           # 中位值滤波器
│   ├── elib_fl_avg.h              # 平均滤波器
│   └── elib_fl_mavg.h             # 滑动平均滤波器
├── src/
│   ├── elib_fl_lpf1_core.h
│   ├── elib_fl_lpf1_core_f.c      # lpf1 float 实现
│   ├── elib_fl_lpf1_core_i32.c    # lpf1 i32 实现
│   ├── elib_fl_lpf1_core_u32.c    # lpf1 u32 实现
│   ├── elib_fl_clip_core.h
│   ├── elib_fl_clip_core_f.c      # clip float 实现
│   ├── elib_fl_clip_core_i32.c    # clip i32 实现
│   ├── elib_fl_clip_core_u32.c    # clip u32 实现
│   ├── elib_fl_median_core.h
│   ├── elib_fl_median_core_f.c    # median float 实现
│   ├── elib_fl_median_core_i32.c  # median i32 实现
│   ├── elib_fl_median_core_u32.c  # median u32 实现
│   ├── elib_fl_avg_core.h
│   ├── elib_fl_avg_core_f.c       # avg float 实现
│   ├── elib_fl_avg_core_i32.c     # avg i32 实现
│   ├── elib_fl_avg_core_u32.c     # avg u32 实现
│   ├── elib_fl_mavg_core.h
│   ├── elib_fl_mavg_core_f.c      # mavg float 实现
│   ├── elib_fl_mavg_core_i32.c    # mavg i32 实现
│   └── elib_fl_mavg_core_u32.c    # mavg u32 实现
├── docs/
│   ├── usage_lpf1.md
│   ├── usage_clip.md
│   ├── usage_median.md
│   ├── usage_avg.md
│   └── usage_mavg.md
├── scripts/
├── LICENSE
└── README.md
```

## 构建

使用 arm-none-eabi-gcc（或任意 C99 编译器），编译全部源文件：

```bash
arm-none-eabi-gcc -std=c99 -Wall -Wextra -Werror -Iinclude -c src/*.c
```

## 许可证

MIT License
