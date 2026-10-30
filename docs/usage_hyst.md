# hyst 模块用法

头文件：`elib_fl.h`

单边界滞回器，零动态分配，纯 C99，提供 float / i16 / i32 / u16 / u32 五套显式接口。

**不做分级**——库只对一条边界返回"带滞回的稳定值"，等级/档位是业务层的事；几个边界就开几个实例。

## 语义（三参数）

以 `s = in − boundary`（boundary 边界值 b，hysteresis 滞回宽度 h，snap 跳变直通宽度 S，init 强制 `h > 0` 且 `S > h`）、out 为最近输出：

**LOW 态（out 在边界的低侧）**

| 条件 | 输出 |
|---|---|
| `s ≤ 0` | 如实返回 `in` |
| `0 < s ≤ h` 且 `\|in − out\| ≤ S` | 粘滞钳位，返回 `b`（最大给到边界） |
| `0 < s ≤ h` 且 `\|in − out\| > S` | **跳变直通**返回 `in`，翻 HIGH |
| `s > h` | 如实返回 `in`，翻 HIGH |

**HIGH 态（曾越过边界+h）**

| 条件 | 输出 |
|---|---|
| `s > 0` | 如实返回 `in` |
| `−h < s ≤ 0` 且 `\|in − out\| ≤ S` | 保持上次输出（回落死区） |
| `−h < s ≤ 0` 且 `\|in − out\| > S` | **跳变直通**返回 `in`，按 in 所在一侧翻态 |
| `s ≤ −h` | 如实返回 `in`，翻 LOW |

要点：

- **粘滞带**：从低侧爬向边界时，越过边界但没超过 `b+h` 的样本一律被钳在 `b`——防止测量值在边界上方打转导致输出抖动。
- **回落死区**：越界后回落 `≤ h` 的样本被保持——要真正翻回 LOW 必须掉到 `b−h` 之下，这就是经典的 Schmitt 滞回。
- **跳变直通（snap）**：`|in − out| > S` 的大步变不被滞回缚住（如 0→6、10→5），按实际值直通。`S > h` 保证滞回信封 `[b−h, b+h]` 内的任何往返永远颠不破它——**snap 管的是真跳变，hysg 管的是抖动**。
- 纯比较无累加：16 位版内部差值走 int32、32 位版走 int64，全量程无溢出面。
- NaN 输入被吞掉（输出保持，状态不动），家族一致。

---

## Float 版本

```c
#include "elib_fl.h"

elib_fl_hyst_ctx_f_t ctx;
elib_fl_hyst_init_f(&ctx, 35.0f, 2.0f, 5.0f);  /* 边界 35，滞回 2，跳变直通 5 */
elib_fl_hyst_set_f(&ctx, 30.0f);               /* 种子值，自动判边 */

float out = elib_fl_hyst_update_f(&ctx, raw);
elib_fl_hyst_reset_f(&ctx);
```

## i16 版本

```c
elib_fl_hyst_ctx_i16_t ctx;
elib_fl_hyst_init_i16(&ctx, 350, 20, 50);      /* 0.1°C 单位 */

int16_t out = elib_fl_hyst_update_i16(&ctx, raw);
elib_fl_hyst_reset_i16(&ctx);
```

## i32 版本

```c
elib_fl_hyst_ctx_i32_t ctx;
elib_fl_hyst_init_i32(&ctx, 350, 20, 50);

int32_t out = elib_fl_hyst_update_i32(&ctx, raw);
elib_fl_hyst_reset_i32(&ctx);
```

## u16 版本

```c
elib_fl_hyst_ctx_u16_t ctx;
elib_fl_hyst_init_u16(&ctx, 400, 20, 50);

uint16_t out = elib_fl_hyst_update_u16(&ctx, raw);
elib_fl_hyst_reset_u16(&ctx);
```

## u32 版本

```c
elib_fl_hyst_ctx_u32_t ctx;
elib_fl_hyst_init_u32(&ctx, 400, 20, 50);

uint32_t out = elib_fl_hyst_update_u32(&ctx, raw);
elib_fl_hyst_reset_u32(&ctx);
```

---

## API

| 版本 | init | set | update | reset |
|------|------|-----|--------|-------|
| float | `elib_fl_hyst_init_f` | `elib_fl_hyst_set_f` | `elib_fl_hyst_update_f` | `elib_fl_hyst_reset_f` |
| i16 | `elib_fl_hyst_init_i16` | `elib_fl_hyst_set_i16` | `elib_fl_hyst_update_i16` | `elib_fl_hyst_reset_i16` |
| i32 | `elib_fl_hyst_init_i32` | `elib_fl_hyst_set_i32` | `elib_fl_hyst_update_i32` | `elib_fl_hyst_reset_i32` |
| u16 | `elib_fl_hyst_init_u16` | `elib_fl_hyst_set_u16` | `elib_fl_hyst_update_u16` | `elib_fl_hyst_reset_u16` |
| u32 | `elib_fl_hyst_init_u32` | `elib_fl_hyst_set_u32` | `elib_fl_hyst_update_u32` | `elib_fl_hyst_reset_u32` |

- `init(ctx, boundary, hyst, snap)`：`hyst` 必须 > 0（0 = 无滞回直通，拒收）；`snap` 必须 > `hyst`（否则滞回信封内的正常抖动会被误判成跳变，拒收）。
- `set(ctx, value)`：种子输出值；HIGH/LOW 由 `value > boundary` 导出。未初始化返 `NOT_INITIALIZED`。
- `update(ctx, in)`：过一遍状态机，返回本次稳定输出。首拍即有效（无窗口/warmup）。
- `reset(ctx)`：清零输出、回 LOW；三个配置参数保留。

---

## 场景：温度分级（风扇 / 报警多门限）

温控典型的三限值 30 °C（开风扇）/ 40 °C（一级报警）/ 50 °C（二级报警）：**每条边界开一个实例**，等级由业务层对三个 above 侧的状态组合得出，库不掺和分级：

```c
#include "elib_fl.h"

elib_fl_hyst_ctx_i16_t fan, warn, alarm;
/* 0.1°C 定标；各自滞回宽度按噪声定（温度高时噪声大，可加宽） */
elib_fl_hyst_init_i16(&fan,   300, 20, 60);   /* 30.0 ±2.0 */
elib_fl_hyst_init_i16(&warn,  400, 30, 60);   /* 40.0 ±3.0 */
elib_fl_hyst_init_i16(&alarm, 500, 50, 80);   /* 50.0 ±5.0 */

int16_t t = smoothed_sample(); /* 平滑后的温度（0.1°C 定标） */

int16_t out_fan   = elib_fl_hyst_update_i16(&fan,   t);
int16_t out_warn  = elib_fl_hyst_update_i16(&warn,  t);
int16_t out_alarm = elib_fl_hyst_update_i16(&alarm, t);
```

典型的等级判据（业务层，利用 ctx 里的 above 位，示意）：

```c
int level =
      (alarm.bit_flags.above ? 3 : 0)
    + (warn.bit_flags.above  ? 2 : 0)
    + (fan.bit_flags.above   ? 1 : 0);
/* 0=正常 1=风扇段 2=风扇+报警 3=风扇+报警+严警 */
```

每条边的输出序列（如 `fan` 实例换算回 °C，边界 30.0，h=2.0，S=6.0，从 25 °C 缓升再骤降）：

```
in : 25 27 29 31 32 33 | 29 28 | 20
out: 25 27 29 30 30 33 | 33 28 | 20   ← 31/32 钳在 30（粘滞带），33 越过 30+2 如实翻 HIGH；
                                        回落 29 保持 33（|Δ| = 4 ≤ 6），28 跌到 30−2 如实翻 LOW，
                                        20 在 LOW 侧如实
```

驱动继电器/风扇只吃返回值（30/31/20），**永远不会有 29.xxx/30.xxx 的开停抖**。

---

## 参数选择指南

| 参数 | 建议 |
|------|------|
| `hyst` | 噪声峰峰宽的 1~2 倍；越小翻级越灵敏但越抖 |
| `snap` | 每拍最大合理速变的 3~5 倍；太小会把正常快速变化当跳变崩掉滞回，太大则真跳变被拖着旧值走 |
| `boundary` | 中性触发点，避开业务临界中夹杂噪声的部分 |

- `snap ≤ hyst` 在 init 被拒——那个约束保证了黏滞带里 `|in − out| ≤ h < S`，正常离合永远不触发直通。
- 想要"跳变后一路直通跟踪"？`snap` 只豁免单拍大步变，之后滞回语义立即恢复；那是 slew（抗尖刺）或直读（不滤波）的活，不是 hyst 的。
- `set()` 适合从已知值（如上一次稳定读数）接续，避免从 0 重新冷启动爬滞回。

## 典型链路

```
median（去毛刺） → mavg/lpf1（平滑） → hyst（逐边界稳定值） → 业务层分级
```

---

> i16/u16 内部差值走 int32（|s|、|d| ≤ 2^16−1 恒不溢出）；i32/u32 走 int64，全量程安全。无窗口、无累加、无动态内存。
