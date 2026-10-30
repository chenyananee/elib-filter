# elib-filter

零动态内存分配的嵌入式 C 滤波算法库，纯 C99，无外部依赖。每个滤波器提供 **float / i16 / i32 / u16 / u32** 五套实现，显式后缀调用，不使用宏分发。单一公共头 `elib_fl.h`，`-Werror` 一等公民。当前版本 `3.0.0`（`ELIB_FL_VERSION`）。

## 用法文档

| 模块 | 文档 |
|------|------|
| lpf1 | [docs/usage_lpf1.md](docs/usage_lpf1.md) |
| slew | [docs/usage_slew.md](docs/usage_slew.md) |
| deadband | [docs/usage_deadband.md](docs/usage_deadband.md) |
| hyst | [docs/usage_hyst.md](docs/usage_hyst.md) |
| median | [docs/usage_median.md](docs/usage_median.md) |
| avg | [docs/usage_avg.md](docs/usage_avg.md) |
| mavg | [docs/usage_mavg.md](docs/usage_mavg.md) |

## 模块列表

唯一公共头：`#include "elib_fl.h"` 引入全部模块（v2 起不再提供子模块头文件）。

| 模块 | 说明 |
|------|------|
| lpf1 | 一阶低通 IIR 滤波器（alpha / 时间常数 / 截止频率三种初始化） |
| slew | 压摆率限幅器（拒绝突变样本，保持上次输出；**不是幅度钳制**，v2 及之前叫 `clip`，v3 正名） |
| deadband | 死区滤波器（忽略阈值内的小变化，保持上次输出；slew 的镜像：小变化保持、大变化跟随） |
| hyst | 单边界滞回器（粘滞带 + 回落死区 + 跳变直通；分级是业务层的事，库只返回稳定值） |
| median | 中位值滤波器（非重叠批窗，每 N 样本出一拍） |
| avg | 平均滤波器（非重叠批窗，每 N 样本出一拍） |
| mavg | 滑动平均滤波器（每样本出一拍，O(1) 更新） |

## 五版本 API

每个滤波器提供五套实现，按类型显式调用对应后缀，纯 C99 可编译：

| 后缀 | 上下文类型 | 说明 |
|------|-----------|------|
| `_f` | `elib_fl_xxx_ctx_f_t` | 单精度浮点 |
| `_i16` | `elib_fl_xxx_ctx_i16_t` | 有符号 16 位 |
| `_i32` | `elib_fl_xxx_ctx_i32_t` | 有符号 32 位 |
| `_u16` | `elib_fl_xxx_ctx_u16_t` | 无符号 16 位 |
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

/* i16 版本 — 同为 Qn 定点，n 取 1..15 */
elib_fl_lpf1_ctx_i16_t ctx_h;
elib_fl_lpf1_init_i16(&ctx_h, 3277, 15);   /* alpha = 0.1 in Q15 */
int16_t out_h = elib_fl_lpf1_update_i16(&ctx_h, 100);
```

lpf1 的整数版本 alpha 为 Qn 定点数：32 位版 `n` 取 1..31，16 位版 `n` 取 1..15；其余滤波器按各自类型直接传参。

## 溢出安全

非法配置全部在 init 一次拒收，库不把非法状态带进运行期。数值侧的保证：

| 类型 | 中途运算宽度 | 保证 |
|------|-------------|------|
| `_f` | float | — |
| `_i32` / `_u32` | 64 位累加器（avg/mavg）、int64 中间量（lpf1）、int64 差值（slew/hyst） | **任意样本值 × 任意窗长**无溢出 |
| `_i16` / `_u16` | 32 位中间量（有界论证） | lpf1/slew：\|α×diff\| ≤ 32767×65535 < 2^31；hyst/deadband：\|diff\| ≤ 2^16−1 恒安全；avg/mavg：init **强制窗长 ≤ 65535**（最坏窗和严格小于 2^31/2^32），更长的窗口在 init 即被拒 |

## 功能列表

### lpf1 — 一阶低通滤波器

> 滤波公式：`y[n] = y[n-1] + α × (x[n] - y[n-1])`

| 函数 | float | 整数五版本 |
|------|-------|-----------|
| `elib_fl_lpf1_init_*(ctx, alpha)` | α: (0.0, 1.0) | alpha: Qn 定点, + n |
| `elib_fl_lpf1_init_tau_*(ctx, tau, dt)` | tau/dt: 秒 | 同左 + n |
| `elib_fl_lpf1_init_fc_*(ctx, fc, fs)` | fc/fs: Hz | 同左 + n |
| `elib_fl_lpf1_update_*(ctx, in)` | float → float | 数值 → 同型 |
| `elib_fl_lpf1_reset_*(ctx)` | ✓ | ✓ |

### slew — 压摆率限幅器

> `|x[n] - y[n-1]| ≤ threshold → y[n] = x[n]`，否则保持 y[n-1]。
> 语义是**拒绝大突变**（保持上次输出），不是对幅度做钳制。v2 及之前叫 `clip`，v3 正名。

| 函数 | float | 整数五版本 |
|------|-------|-----------|
| `elib_fl_slew_init_*(ctx, threshold)` | float | 数值，须 > 0 |
| `elib_fl_slew_set_*(ctx, value)` | float | 数值；未初始化返 `NOT_INITIALIZED` |
| `elib_fl_slew_update_*(ctx, in)` | float → float | 数值 → 同型（16/32 位版差值走 int32/int64，满量程安全） |
| `elib_fl_slew_reset_*(ctx)` | ✓ | ✓ |

### deadband — 死区滤波器

> `|x[n] - y[n-1]| ≤ threshold → 保持 y[n-1]`，否则跟随 `x[n]`。
> slew 的镜像：**小变化被忽略**（抗抖动），大变化原样放行。死区含边界（`≤` 保持）。

| 函数 | float | 整数五版本 |
|------|-------|-----------|
| `elib_fl_deadband_init_*(ctx, threshold)` | float | 数值，须 > 0（0 等价于直通，init 拒收） |
| `elib_fl_deadband_set_*(ctx, value)` | float | 数值；未初始化返 `NOT_INITIALIZED` |
| `elib_fl_deadband_update_*(ctx, in)` | float → float | 数值 → 同型（16/32 位版差值走 int32/int64，满量程安全） |
| `elib_fl_deadband_reset_*(ctx)` | ✓ | ✓ |

无窗口、无 warmup 概念：首个 update 即有效输出。

### hyst — 单边界滞回器

> 一条决策边界 `boundary`，三参数：滞回宽度 `hyst`（> 0）、跳变直通宽度 `snap`（> hyst，init 强制）。
> 爬向边界的样本在 `boundary < in ≤ boundary + hyst` 内被**钳在 boundary**；越界后回落 `≤ hyst` 被**保持**（Schmitt 滞回）；`|in − out| > snap` 的真跳变**原样直通**。

| 函数 | float | 整数五版本 |
|------|-------|-----------|
| `elib_fl_hyst_init_*(ctx, boundary, hyst, snap)` | float | 数值；`hyst > 0` 且 `snap > hyst`，违规 init 拒收 |
| `elib_fl_hyst_set_*(ctx, value)` | float | 种子值（自动判 HIGH/LOW）；未初始化返 `NOT_INITIALIZED` |
| `elib_fl_hyst_update_*(ctx, in)` | float → float | 数值 → 同型（NaN 吞掉保持输出） |
| `elib_fl_hyst_reset_*(ctx)` | ✓ | ✓ |

**不做分级**：每条边界一个实例；等级由业务层对多个实例的 `above` 位组合得出。

### median — 中位值滤波器

> 收集 N 个样本排序取中位值（偶数窗取排序后 `buf[size/2]`，即较大中位元素），非重叠窗口。
> **注意**：排序在用户缓冲区**原地**进行，每出一拍缓冲区内容即被排序破坏（下一批反正要覆盖它，通常无害，但需知情）。

| 函数 | float | 整数五版本 |
|------|-------|-----------|
| `elib_fl_median_init_*(ctx, buf, size)` | float* | 数值指针（用户持有、自备） |
| `elib_fl_median_update_*(ctx, in)` | float → float | 数值 → 同型 |
| `elib_fl_median_ready_*(ctx)` | ✓ | ✓ |
| `elib_fl_median_reset_*(ctx)` | ✓ | ✓ |

### avg — 平均滤波器

> 收集 N 个样本求均值，非重叠窗口。整数版向上取整方式：向零截断（C99 整除语义）。

| 函数 | float | 整数五版本 |
|------|-------|-----------|
| `elib_fl_avg_init_*(ctx, size)` | ✓ | ✓（i16/u16 版 size ≤ 65535，超限拒收） |
| `elib_fl_avg_update_*(ctx, in)` | float → float | 数值 → 同型 |
| `elib_fl_avg_ready_*(ctx)` | ✓ | ✓ |
| `elib_fl_avg_reset_*(ctx)` | ✓ | ✓ |

### mavg — 滑动平均滤波器

> 滑动窗口，O(1) 更新，每样本出一拍；填窗期返回部分均值，填满后恒为整窗均值。init 会把用户缓冲区清零；reset 不回清缓冲区（窗口逻辑在重新填满前不会读旧内容）。

| 函数 | float | 整数五版本 |
|------|-------|-----------|
| `elib_fl_mavg_init_*(ctx, buf, size)` | float* | 数值指针（i16/u16 版 size ≤ 65535，超限拒收） |
| `elib_fl_mavg_update_*(ctx, in)` | float → float | 数值 → 同型 |
| `elib_fl_mavg_ready_*(ctx)` | ✓ | ✓ |
| `elib_fl_mavg_reset_*(ctx)` | ✓ | ✓ |

### v1 → v2 迁移对照

| v1 | v2 | 说明 |
|----|----|------|
| `elib_fl_*_warmup_*` | `elib_fl_*_ready_*` | 返回值语义不变（0/1）；名字如实 |
| `avg/mavg` 32 位累加器 | 64 位累加器 | **ctx 内存布局变更**（`sum` 字段类型），i32/u32 任意样本×任意窗长无溢出 |
| 超界窗长静默 UB/回绕 | init 拒收 | 非法配置不进运行期 |
| `lpf1_i32` α 舍入缺口 | 补 `scaled >= 2^n` 拒收 | 对齐 u32 姊妹版 |
| 6 子头 + 5 内部壳头 | 唯一 `elib_fl.h` | 子头删除，所有 `.c` 直连伞头 |
| init 不清 ctx | init memset ctx | ctx 全量不变量 |
| — | 新增 `_i16`/`_u16` 五版本 | 32 位中途 + 窗长上限严防溢出 |
| — | 新增 `deadband` 模块 | 死区滤波（slew 的镜像） |

### v2 → v3 迁移对照

| v2 | v3 | 说明 |
|----|----|------|
| `elib_fl_clip_*` | `elib_fl_slew_*` | 正名：压摆率限幅器。类型 10 个、函数 20 个全量改名，语义与参数不变；无兼容别名 |
| — | 新增 `hyst` 模块 | 单边界滞回器（boundary/hyst/snap 三参数），七模块 × 五版本 = 35 源；公共 API 破坏性变化随改名一并记入主版本 `3.0.0` |

未满窗口 / 未初始化时 update 返回 0（哨兵语义，0 是合法输出时的歧义用 `ready()` 消解）——v2 文档钉死该行为，不新增错误通道。

### 错误码

| 错误码 | 说明 |
|--------|------|
| `ELIB_FL_OK` | 成功 |
| `ELIB_FL_ERR_INVALID_PARAM` | 无效参数（NULL、空窗、死区 alpha 等，init 一次拒收） |
| `ELIB_FL_ERR_NOT_INITIALIZED` | 未初始化 |

## 目录结构

```
elib-filter/
├── include/
│   └── elib_fl.h                  # 唯一公共头（错误码 + 七模块五版本）
├── src/
│   ├── elib_fl_lpf1_core_f.c      # lpf1 五版本实现
│   ├── elib_fl_lpf1_core_i16.c
│   ├── elib_fl_lpf1_core_i32.c
│   ├── elib_fl_lpf1_core_u16.c
│   ├── elib_fl_lpf1_core_u32.c
│   ├── elib_fl_slew_core_f.c        # slew 五版本实现（v2 的 clip 改名）
│   ├── elib_fl_slew_core_i16.c
│   ├── elib_fl_slew_core_i32.c
│   ├── elib_fl_slew_core_u16.c
│   ├── elib_fl_slew_core_u32.c
│   ├── elib_fl_deadband_core_f.c    # deadband 五版本实现
│   ├── elib_fl_deadband_core_i16.c
│   ├── elib_fl_deadband_core_i32.c
│   ├── elib_fl_deadband_core_u16.c
│   ├── elib_fl_deadband_core_u32.c
│   ├── elib_fl_hyst_core_f.c        # hyst 五版本实现
│   ├── elib_fl_hyst_core_i16.c
│   ├── elib_fl_hyst_core_i32.c
│   ├── elib_fl_hyst_core_u16.c
│   ├── elib_fl_hyst_core_u32.c
│   ├── elib_fl_median_core_f.c    # median 五版本实现
│   ├── elib_fl_median_core_i16.c
│   ├── elib_fl_median_core_i32.c
│   ├── elib_fl_median_core_u16.c
│   ├── elib_fl_median_core_u32.c
│   ├── elib_fl_avg_core_f.c       # avg 五版本实现
│   ├── elib_fl_avg_core_i16.c
│   ├── elib_fl_avg_core_i32.c
│   ├── elib_fl_avg_core_u16.c
│   ├── elib_fl_avg_core_u32.c
│   ├── elib_fl_mavg_core_f.c      # mavg 五版本实现
│   ├── elib_fl_mavg_core_i16.c
│   ├── elib_fl_mavg_core_i32.c
│   ├── elib_fl_mavg_core_u16.c
│   └── elib_fl_mavg_core_u32.c
├── test/
│   ├── test_elib_fl_lpf1.c
│   ├── test_elib_fl_slew.c
│   ├── test_elib_fl_deadband.c
│   ├── test_elib_fl_hyst.c
│   ├── test_elib_fl_median.c
│   ├── test_elib_fl_avg.c
│   └── test_elib_fl_mavg.c
├── docs/
│   ├── usage_lpf1.md
│   ├── usage_slew.md
│   ├── usage_deadband.md
│   ├── usage_hyst.md
│   ├── usage_median.md
│   ├── usage_avg.md
│   └── usage_mavg.md
├── scripts/
├── LICENSE
└── README.md
```

## 构建

无构建系统，单一编译命令（arm-none-eabi-gcc 或任意 C99 编译器）：

```bash
arm-none-eabi-gcc -std=c99 -Wall -Wextra -Werror -Iinclude -c src/*.c
```

编译单元各自直连 `elib_fl.h`（`-Iinclude`，无相对路径插入），多文件工程按任意组合链接 `.o` 即可。

## 许可证

MIT License
