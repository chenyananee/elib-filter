# mavg C99 三版本重构 — 设计文档

日期：2026-09-11
范围：仅滑动平均滤波器（mavg），作为整库 C99 + 三版本迁移的先行样板。

## 背景与目标

现有库为 C11，靠 `_Generic` 在 float / Q32 之间自动分发；mavg 同时存在
`float` 与 `Q32` 两版，含 `oneshot`。本次将 mavg 重构为：

- **纯 C99**（不用 `_Generic`，显式后缀函数）
- **三版本**：float / i32 / u32
- 对外仅提供 `init` / `update` / `warmup` / `reset` 四个接口，删除 `oneshot`
- 架构不变：ctx 句柄，用户提供缓冲区，内部零动态内存
- 旧 `_q32` 改名为 `_i32`，不保留兼容别名

其余模块（lpf1 / clip / median / avg）暂不改动，后续按同一模板逐个迁移。

## 命名约定

| 版本 | 后缀 | 上下文类型 |
|------|------|-----------|
| 单精度浮点 | `_f` | `elib_fl_mavg_ctx_f_t` |
| 有符号 32 位 | `_i32` | `elib_fl_mavg_ctx_i32_t` |
| 无符号 32 位 | `_u32` | `elib_fl_mavg_ctx_u32_t` |

函数命名：`elib_fl_mavg_<op>_<suffix>`，如 `elib_fl_mavg_update_i32`。

## 对外接口（`include/elib_fl_mavg.h`）

```c
typedef struct {
    float    *buf;      /* 用户提供 */
    float     sum;
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct { uint8_t initialized : 1; } bit_flags;
} elib_fl_mavg_ctx_f_t;

typedef struct {
    int32_t  *buf;
    int32_t   sum;
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct { uint8_t initialized : 1; } bit_flags;
} elib_fl_mavg_ctx_i32_t;

typedef struct {
    uint32_t *buf;
    uint32_t  sum;
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct { uint8_t initialized : 1; } bit_flags;
} elib_fl_mavg_ctx_u32_t;

elib_fl_err_t elib_fl_mavg_init_f  (elib_fl_mavg_ctx_f_t *ctx, float *buf, uint32_t size);
float         elib_fl_mavg_update_f(elib_fl_mavg_ctx_f_t *ctx, float in);
uint32_t      elib_fl_mavg_warmup_f(const elib_fl_mavg_ctx_f_t *ctx);
void          elib_fl_mavg_reset_f (elib_fl_mavg_ctx_f_t *ctx);
/* _i32 / _u32 同构，返回类型分别为 int32_t / uint32_t */
```

删除：`oneshot` 三函数、全部 `_Generic` 宏。

## 语义

| 接口 | 行为 |
|------|------|
| `init` | 校验 `ctx/buf/size`（任一 NULL 或 size==0 → `ELIB_FL_ERR_INVALID_PARAM`）；存 `buf`/`size`；`sum=0, idx=0, count=0`；`initialized=1`；**buf 清零**（O(N)，确定性初值）。 |
| `update` | NULL 或未初始化 → 返回 0。若 `count >= size` 先 `sum -= buf[idx]`，否则 `count++`；写 `buf[idx]=in`、`sum += in`、`idx=(idx+1)%size`；返回 `sum/count`（预热期返回部分均值，始终是一个数值）。`count` 封顶 `size`，恒 ≥1，永不除零。 |
| `warmup` | NULL 或未初始化 → 0。返回就绪标志：`count >= size` 返回 **1**，否则 **0**。`count` 封顶 `size`，故首次填满后恒为 1；`reset` 后归 0。返回类型 `uint32_t`。 |
| `reset` | NULL 或未初始化 → 空操作。`sum=0, idx=0, count=0`，**不清 buf**（O(1)）。因 `count<size` 时不触发减法，旧数据不会被误减；重填时逐格覆盖，安全。 |

判断依据：`warmup(ctx) == 1` → 窗口已满、`update` 返回完整窗口均值；`== 0` → 预热中，返回值为部分均值。

### 溢出边界（写入文档）

`sum` 与样本同宽：

- i32：窗口内 `|样本|` 之和 ≤ `INT32_MAX`
- u32：窗口内样本之和 ≤ `UINT32_MAX`

因先减后加，峰值不超过满窗口和，满足上述边界即安全。

## 实现策略

**方案 A：三份独立手写实现**（已选定）。`_core_f.c` / `_core_i32.c` /
`_core_u32.c` 各一份，逻辑相同、类型不同，命名与结构对称。不采用 X-macro
模板，也不采用 `void*` 泛型核心。

## 文件变更

- 重命名 `src/elib_fl_mavg_core.c` → `src/elib_fl_mavg_core_f.c`（float）
- 重命名 `src/elib_fl_mavg_core_q32.c` → `src/elib_fl_mavg_core_i32.c`，改型并重命名函数
- 新增 `src/elib_fl_mavg_core_u32.c`
- `src/elib_fl_mavg_core.h` 不变（仅 include 公共头）
- 改 `include/elib_fl_mavg.h`
- 改 `docs/usage_mavg.md`（三版本示例 + 溢出边界 + warmup 语义）
- 改 `README.md`（mavg 功能表、目录树、构建命令行改为 `-std=c99`，去掉 oneshot 引用）

## 验证

- 用本机 MSVC（无 gcc/clang）以 `/std:c11 /W4` 编译（C99 代码同时是合法 C11），目标零警告。
- 一次性验证程序覆盖：
  - float / i32 / u32 各自的预热计数（`warmup` 由 0 变 1 的时机）
  - 满窗口均值正确
  - `warmup==1` 稳态标志
  - 溢出边界样例
  - `reset` 后重填正确
  - NULL / 未初始化防护

不新建长期测试目录（仓库现无测试基建）；如需可后续补 `test/`。

## 明确排除（本次不做）

- 不迁移 lpf1 / clip / median / avg（后续按同一模板逐个做）
- 不新增 `oneshot`
- 不新增动态内存、不引入依赖
