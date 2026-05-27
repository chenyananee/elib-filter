# elib-filter

零动态内存分配的嵌入式 C 滤波算法库，纯 C11，无外部依赖。支持 float 和 Q32 定点数双版本，通过 C11 `_Generic` 宏统一 API。

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

## 双版本 API

每个滤波器同时提供 **float** 和 **Q32 定点数** 两套实现，通过 C11 `_Generic` 宏自动分发：

```c
/* float 版本 — 使用 elib_fl_xxx_ctx_f_t */
elib_fl_lpf1_ctx_f_t ctx_f;
elib_fl_lpf1_init(&ctx_f, 0.1f);           /* 自动调用 _f 版本 */
float out_f = elib_fl_lpf1_update(&ctx_f, 3.14f);

/* Q32 版本 — 使用 elib_fl_xxx_ctx_q32_t */
elib_fl_lpf1_ctx_q32_t ctx_q;
elib_fl_lpf1_init(&ctx_q, 6554, 16);       /* 自动调用 _q32 版本 */
int32_t out_q = elib_fl_lpf1_update(&ctx_q, 100);
```

同一程序中可以同时使用两种版本，编译器自动根据 ctx 类型选择正确的实现。

## 功能列表

### lpf1 — 一阶低通滤波器

> 滤波公式：`y[n] = y[n-1] + α × (x[n] - y[n-1])`

| 函数 | float | Q32 |
|------|-------|-----|
| `elib_fl_lpf1_init(ctx, alpha)` | α: (0.0, 1.0) | alpha: Qn 格式, n: 小数位数 |
| `elib_fl_lpf1_init_tau(ctx, tau, dt)` | tau/dt: 秒 | 同左 + n |
| `elib_fl_lpf1_init_fc(ctx, fc, fs)` | fc/fs: Hz | 同左 + n |
| `elib_fl_lpf1_update(ctx, in)` | float → float | int32 → int32 |
| `elib_fl_lpf1_reset(ctx)` | ✓ | ✓ |

### clip — 限幅滤波器

> `|x[n] - y[n-1]| ≤ threshold → y[n] = x[n]`，否则保持 y[n-1]

| 函数 | float | Q32 |
|------|-------|-----|
| `elib_fl_clip_init(ctx, threshold)` | float | int32 + n |
| `elib_fl_clip_set(ctx, value)` | float | int32 |
| `elib_fl_clip_update(ctx, in)` | float → float | int32 → int32 |
| `elib_fl_clip_reset(ctx)` | ✓ | ✓ |

### median — 中位值滤波器

> 收集 N 个样本，排序取中位值，非重叠窗口

| 函数 | float | Q32 |
|------|-------|-----|
| `elib_fl_median_init(ctx, buf, size)` | float* | int32_t* |
| `elib_fl_median_update(ctx, in)` | float → float | int32 → int32 |
| `elib_fl_median_reset(ctx)` | ✓ | ✓ |

### avg — 平均滤波器

> 收集 N 个样本求均值，非重叠窗口

| 函数 | float | Q32 |
|------|-------|-----|
| `elib_fl_avg_init(ctx, size)` | ✓ | ✓ |
| `elib_fl_avg_update(ctx, in)` | float → float | int32 → int32 |
| `elib_fl_avg_reset(ctx)` | ✓ | ✓ |

### mavg — 滑动平均滤波器

> 滑动窗口，O(1) 更新，自动预热输出

| 函数 | float | Q32 |
|------|-------|-----|
| `elib_fl_mavg_init(ctx, buf, size)` | float* | int32_t* |
| `elib_fl_mavg_update(ctx, in)` | float → float | int32 → int32 |
| `elib_fl_mavg_warmup(ctx)` | ✓ | ✓ |
| `elib_fl_mavg_reset(ctx)` | ✓ | ✓ |

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
│   ├── elib_fl_lpf1_core.c        # lpf1 float 实现
│   ├── elib_fl_lpf1_core.h
│   ├── elib_fl_lpf1_core_q32.c    # lpf1 Q32 实现
│   ├── elib_fl_clip_core.c        # clip float 实现
│   ├── elib_fl_clip_core.h
│   ├── elib_fl_clip_core_q32.c    # clip Q32 实现
│   ├── elib_fl_median_core.c      # median float 实现
│   ├── elib_fl_median_core.h
│   ├── elib_fl_median_core_q32.c  # median Q32 实现
│   ├── elib_fl_avg_core.c         # avg float 实现
│   ├── elib_fl_avg_core.h
│   ├── elib_fl_avg_core_q32.c     # avg Q32 实现
│   ├── elib_fl_mavg_core.c        # mavg float 实现
│   ├── elib_fl_mavg_core.h
│   └── elib_fl_mavg_core_q32.c    # mavg Q32 实现
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

```bash
gcc -std=c11 -Wall -Wextra -Iinclude -o test_elib_fl test/test_elib_fl.c \
  src/elib_fl_lpf1_core.c src/elib_fl_lpf1_core_q32.c \
  src/elib_fl_clip_core.c src/elib_fl_clip_core_q32.c \
  src/elib_fl_median_core.c src/elib_fl_median_core_q32.c \
  src/elib_fl_avg_core.c src/elib_fl_avg_core_q32.c \
  src/elib_fl_mavg_core.c src/elib_fl_mavg_core_q32.c && ./test_elib_fl
```

## 许可证

MIT License
