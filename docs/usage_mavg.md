# mavg 模块用法

头文件：`elib_fl.h`

滑动平均滤波器，零动态内存，滑动窗口，O(1) 更新。提供 float / i16 / i32 / u16 / u32 五套实现，纯 C99，无宏分发，按类型显式调用对应函数。

---

## Float 版本

```c
#include "elib_fl.h"

float buf[8];
elib_fl_mavg_ctx_f_t ctx;
elib_fl_mavg_init_f(&ctx, buf, 8);

while (1) {
    float out = elib_fl_mavg_update_f(&ctx, read_sensor());
    if (elib_fl_mavg_ready_f(&ctx)) {
        use_output(out);  /* 窗口已满，out 为完整窗口均值 */
    }
}

elib_fl_mavg_reset_f(&ctx);
```

## i16 版本

```c
int16_t buf[8];
elib_fl_mavg_ctx_i16_t ctx;
elib_fl_mavg_init_i16(&ctx, buf, 8);   /* size ≤ 65535，超限 init 拒收 */

while (1) {
    int16_t out = elib_fl_mavg_update_i16(&ctx, read_adc_i16());
    if (elib_fl_mavg_ready_i16(&ctx)) {
        use_output(out);
    }
}
```

## i32 版本

```c
int32_t buf[8];
elib_fl_mavg_ctx_i32_t ctx;
elib_fl_mavg_init_i32(&ctx, buf, 8);   /* 64 位累加器，任意窗长 */

while (1) {
    int32_t out = elib_fl_mavg_update_i32(&ctx, read_adc());
    if (elib_fl_mavg_ready_i32(&ctx)) {
        use_output(out);
    }
}
```

## u16 版本

```c
uint16_t buf[8];
elib_fl_mavg_ctx_u16_t ctx;
elib_fl_mavg_init_u16(&ctx, buf, 8);   /* size ≤ 65535，超限 init 拒收 */

while (1) {
    uint16_t out = elib_fl_mavg_update_u16(&ctx, read_counter16());
    if (elib_fl_mavg_ready_u16(&ctx)) {
        use_output(out);
    }
}
```

## u32 版本

```c
uint32_t buf[8];
elib_fl_mavg_ctx_u32_t ctx;
elib_fl_mavg_init_u32(&ctx, buf, 8);   /* 64 位累加器，任意窗长 */

while (1) {
    uint32_t out = elib_fl_mavg_update_u32(&ctx, read_counter());
    if (elib_fl_mavg_ready_u32(&ctx)) {
        use_output(out);
    }
}
```

---

## API

| 版本 | init | update | ready | reset |
|------|------|--------|-------|-------|
| float | `elib_fl_mavg_init_f` | `elib_fl_mavg_update_f` | `elib_fl_mavg_ready_f` | `elib_fl_mavg_reset_f` |
| i16 | `elib_fl_mavg_init_i16` | `elib_fl_mavg_update_i16` | `elib_fl_mavg_ready_i16` | `elib_fl_mavg_reset_i16` |
| i32 | `elib_fl_mavg_init_i32` | `elib_fl_mavg_update_i32` | `elib_fl_mavg_ready_i32` | `elib_fl_mavg_reset_i32` |
| u16 | `elib_fl_mavg_init_u16` | `elib_fl_mavg_update_u16` | `elib_fl_mavg_ready_u16` | `elib_fl_mavg_reset_u16` |
| u32 | `elib_fl_mavg_init_u32` | `elib_fl_mavg_update_u32` | `elib_fl_mavg_ready_u32` | `elib_fl_mavg_reset_u32` |

- `init(ctx, buf, size)`：绑定用户提供的缓冲区 `buf`（长度 `size`），清零缓冲区，返回 `ELIB_FL_OK` 或 `ELIB_FL_ERR_INVALID_PARAM`（i16/u16 版另要求 size ≤ 65535）。
- `update(ctx, in)`：压入样本，返回当前窗口均值；窗口未满时返回部分均值（始终是一个数值）。
- `ready(ctx)`：就绪标志，窗口未满返回 `0`，首次填满后恒为 `1`（`reset` 后归 `0`）。v1 的 `warmup_*` 在 v2 改名为 `ready_*`，0/1 语义不变。
- `reset(ctx)`：清空状态，O(1)，不清缓冲区（旧内容在窗口重新填满前不会被读取）。

> **溢出边界（v2）**：i32/u32 版累加器为 64 位，**任意样本值 × 任意窗长**均合法（v1 的 32 位累加器 UB/回绕已消除）。i16/u16 版累加器为 32 位，最坏窗和以 init 强制 `size ≤ 65535` 保证恒不溢出。

---

## avg 与 mavg 对比

| 特性 | avg（平均） | mavg（滑动平均） |
|------|------------|-----------------|
| 窗口模式 | 非重叠，窗口满后清空 | 滑动，持续更新 |
| 输出频率 | 每 N 个样本输出 1 次 | 每个样本都输出 |
| 实时性 | 低 | 高 |

适用于：需要实时逐样本输出的平滑场景，如传感器数据实时处理、控制回路滤波。
