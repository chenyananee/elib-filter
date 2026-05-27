# mavg 模块用法

头文件：`elib_fl_mavg.h`（通过 `elib_fl.h` 自动引入）

滑动平均滤波器，零动态分配，滑动窗口，O(1) 更新，支持 float 和 Q32 两种版本。

---

## Float 版本

```c
#include "elib_fl.h"

float buf[8];
elib_fl_mavg_ctx_f_t ctx;
elib_fl_mavg_init(&ctx, buf, 8);

while (1) {
    float out = elib_fl_mavg_update(&ctx, read_sensor());
    use_output(out);  /* 始终有效 */
}

uint32_t remain = elib_fl_mavg_warmup(&ctx);  /* 预热剩余 */
elib_fl_mavg_reset(&ctx);
```

## Q32 版本

```c
int32_t buf[8];
elib_fl_mavg_ctx_q32_t ctx;
elib_fl_mavg_init(&ctx, buf, 8);

while (1) {
    int32_t out = elib_fl_mavg_update(&ctx, read_adc());
    use_output(out);
}
```

> Q32 版本使用 int32_t 运行和，注意溢出限制：`窗口内采样值之和 ≤ INT32_MAX`，即单个采样值 × 窗口大小不超过 2,147,483,647。
> 例：窗口 32，采样值上限 = 2,147,483,647 / 32 = 67,108,863。

---

## Generic API

| 宏 | 分发依据 |
|----|---------|
| `elib_fl_mavg_init(ctx, ...)` | ctx 类型 |
| `elib_fl_mavg_update(ctx, in)` | ctx 类型 |
| `elib_fl_mavg_warmup(ctx)` | ctx 类型 |
| `elib_fl_mavg_reset(ctx)` | ctx 类型 |

---

## avg 与 mavg 对比

| 特性 | avg（平均） | mavg（滑动平均） |
|------|------------|-----------------|
| 窗口模式 | 非重叠，窗口满后清空 | 滑动，持续更新 |
| 输出频率 | 每 N 个样本输出 1 次 | 每个样本都输出 |
| 实时性 | 低 | 高 |

适用于：需要实时逐样本输出的平滑场景，如传感器数据实时处理、控制回路滤波。
