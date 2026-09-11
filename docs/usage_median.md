# median 模块用法

头文件：`elib_fl_median.h`（通过 `elib_fl.h` 自动引入）

中位值滤波器，零动态分配，非重叠窗口，纯 C99，提供 float / i32 / u32 三套显式接口。

---

## Float 版本

```c
#include "elib_fl.h"

float buf[5];
elib_fl_median_ctx_f_t ctx;
elib_fl_median_init_f(&ctx, buf, 5);

while (1) {
    float out = elib_fl_median_update_f(&ctx, read_sensor());
    if (elib_fl_median_warmup_f(&ctx)) {
        use_output(out);  /* 本次凑满窗口，out 为有效中位值 */
    }
}

elib_fl_median_reset_f(&ctx);
```

## i32 版本

```c
int32_t buf[5];
elib_fl_median_ctx_i32_t ctx;
elib_fl_median_init_i32(&ctx, buf, 5);

while (1) {
    int32_t out = elib_fl_median_update_i32(&ctx, read_adc());
    if (elib_fl_median_warmup_i32(&ctx)) {
        use_output(out);
    }
}
```

## u32 版本

```c
uint32_t buf[5];
elib_fl_median_ctx_u32_t ctx;
elib_fl_median_init_u32(&ctx, buf, 5);

while (1) {
    uint32_t out = elib_fl_median_update_u32(&ctx, read_counter());
    if (elib_fl_median_warmup_u32(&ctx)) {
        use_output(out);
    }
}
```

---

## API

| 版本 | init | update | warmup | reset |
|------|------|--------|--------|-------|
| float | `elib_fl_median_init_f` | `elib_fl_median_update_f` | `elib_fl_median_warmup_f` | `elib_fl_median_reset_f` |
| i32 | `elib_fl_median_init_i32` | `elib_fl_median_update_i32` | `elib_fl_median_warmup_i32` | `elib_fl_median_reset_i32` |
| u32 | `elib_fl_median_init_u32` | `elib_fl_median_update_u32` | `elib_fl_median_warmup_u32` | `elib_fl_median_reset_u32` |

- `init(ctx, buf, size)`：绑定用户提供的缓冲区 `buf`（长度 `size`），返回 `ELIB_FL_OK` 或 `ELIB_FL_ERR_INVALID_PARAM`。
- `update(ctx, in)`：压入样本；窗口未满返回 0，满则排序取中位值并清空窗口。
- `warmup(ctx)`：就绪标志。仅当最近一次 `update` 凑满窗口、返回值为有效中位值时为 `1`，否则为 `0`。
- `reset(ctx)`：清空窗口计数，O(1)，不清缓冲区。

---

## 参数选择指南

| 窗口大小 | 效果 |
|----------|------|
| 3 | 轻度过滤，响应快 |
| 5 | 通用场景，推荐 |
| 7 | 较强过滤，延迟增大 |
| 9+ | 强过滤，适用于高脉冲噪声 |

适用于：消除椒盐噪声、传感器脉冲干扰、ADC 毛刺。

> 偶数窗口取 `buf[size/2]`（偏大的中间元素）。
