[简体中文](README.md) | **English**

# elib-filter

A zero-dynamic-memory embedded C filter library, pure C99, no external dependencies. Every filter ships five implementations — **float / i16 / i32 / u16 / u32** — called through explicit suffixes, with no macro dispatch. A single public header `elib_fl.h`, and `-Werror` is a first-class citizen. Current version `3.0.0` (`ELIB_FL_VERSION`).

## Usage Docs

| Module | Document |
|------|------|
| lpf1 | [docs/usage_lpf1.md](docs/usage_lpf1.md) |
| slew | [docs/usage_slew.md](docs/usage_slew.md) |
| deadband | [docs/usage_deadband.md](docs/usage_deadband.md) |
| hyst | [docs/usage_hyst.md](docs/usage_hyst.md) |
| median | [docs/usage_median.md](docs/usage_median.md) |
| avg | [docs/usage_avg.md](docs/usage_avg.md) |
| mavg | [docs/usage_mavg.md](docs/usage_mavg.md) |

## Modules

Single public header: `#include "elib_fl.h"` pulls in every module (no per-module headers since v2).

| Module | Description |
|------|------|
| lpf1 | First-order low-pass IIR filter (three ways to initialize: alpha / time constant / cutoff frequency) |
| slew | Slew-rate limiter (rejects abrupt samples and holds the previous output; **not amplitude clamping**; named `clip` up to v2, renamed in v3) |
| deadband | Deadband filter (ignores small changes inside the threshold and holds the previous output; the mirror of slew: small changes hold, large changes follow) |
| hyst | Single-boundary hysteresis (sticky band + fall-back deadband + snap-through; grading belongs to the application layer, the library only returns the stable value) |
| median | Median filter (non-overlapping batch windows, one output every N samples) |
| avg | Average filter (non-overlapping batch windows, one output every N samples) |
| mavg | Moving average filter (one output per sample, O(1) update) |

## Five-Variant API

Every filter provides five implementations; call the suffix that matches the type explicitly; all of them compile as pure C99:

| Suffix | Context type | Description |
|------|-----------|------|
| `_f` | `elib_fl_xxx_ctx_f_t` | Single-precision float |
| `_i16` | `elib_fl_xxx_ctx_i16_t` | Signed 16-bit |
| `_i32` | `elib_fl_xxx_ctx_i32_t` | Signed 32-bit |
| `_u16` | `elib_fl_xxx_ctx_u16_t` | Unsigned 16-bit |
| `_u32` | `elib_fl_xxx_ctx_u32_t` | Unsigned 32-bit |

```c
/* float version */
elib_fl_lpf1_ctx_f_t ctx_f;
elib_fl_lpf1_init_f(&ctx_f, 0.1f);
float out_f = elib_fl_lpf1_update_f(&ctx_f, 3.14f);

/* i32 version — alpha is a Qn fixed-point number, n is the number of fractional bits */
elib_fl_lpf1_ctx_i32_t ctx_q;
elib_fl_lpf1_init_i32(&ctx_q, 6554, 16);   /* alpha = 0.1 in Q16 */
int32_t out_q = elib_fl_lpf1_update_i32(&ctx_q, 100);

/* i16 version — also Qn fixed point, n in 1..15 */
elib_fl_lpf1_ctx_i16_t ctx_h;
elib_fl_lpf1_init_i16(&ctx_h, 3277, 15);   /* alpha = 0.1 in Q15 */
int16_t out_h = elib_fl_lpf1_update_i16(&ctx_h, 100);
```

For the integer variants of lpf1, alpha is a Qn fixed-point number: `n` is 1..31 in the 32-bit variants, and `n` is 1..15 in the 16-bit variants; the other filters take their parameters directly in their own type.

## Overflow Safety

Illegal configurations are all rejected once at init; the library never carries an illegal state into run time. Numeric guarantees:

| Type | Intermediate operation width | Guarantee |
|------|-------------|------|
| `_f` | float | — |
| `_i32` / `_u32` | 64-bit accumulator (avg/mavg), int64 intermediate (lpf1), int64 difference (slew/hyst) | **any sample value × any window length** without overflow |
| `_i16` / `_u16` | 32-bit intermediates (bounded by proof) | lpf1/slew: \|α×diff\| ≤ 32767×65535 < 2^31; hyst/deadband: \|diff\| ≤ 2^16−1 is always safe; avg/mavg: init **enforces window length ≤ 65535** (the worst-case window sum is strictly below 2^31/2^32), so longer windows are rejected at init |

## Module Details

### lpf1 — First-Order Low-Pass Filter

> Filter equation: `y[n] = y[n-1] + α × (x[n] - y[n-1])`

| Function | float | Integer five-variant |
|------|-------|-----------|
| `elib_fl_lpf1_init_*(ctx, alpha)` | α: (0.0, 1.0) | alpha: Qn fixed point, + n |
| `elib_fl_lpf1_init_tau_*(ctx, tau, dt)` | tau/dt: seconds | as left + n |
| `elib_fl_lpf1_init_fc_*(ctx, fc, fs)` | fc/fs: Hz | as left + n |
| `elib_fl_lpf1_update_*(ctx, in)` | float → float | value → same type |
| `elib_fl_lpf1_reset_*(ctx)` | ✓ | ✓ |

### slew — Slew-Rate Limiter

> `|x[n] - y[n-1]| ≤ threshold → y[n] = x[n]`, otherwise hold y[n-1].
> The semantics are **rejecting large jumps** (hold the previous output), not clamping the amplitude. Named `clip` up to v2, renamed in v3.

| Function | float | Integer five-variant |
|------|-------|-----------|
| `elib_fl_slew_init_*(ctx, threshold)` | float | value, must be > 0 |
| `elib_fl_slew_set_*(ctx, value)` | float | value; returns `NOT_INITIALIZED` when not initialized |
| `elib_fl_slew_update_*(ctx, in)` | float → float | value → same type (the 16/32-bit variants compute the difference in int32/int64, full-scale safe) |
| `elib_fl_slew_reset_*(ctx)` | ✓ | ✓ |

### deadband — Deadband Filter

> `|x[n] - y[n-1]| ≤ threshold → hold y[n-1]`, otherwise follow `x[n]`.
> The mirror of slew: **small changes are ignored** (anti-jitter) and large changes pass through unchanged. The deadband includes its boundary (`≤` holds).

| Function | float | Integer five-variant |
|------|-------|-----------|
| `elib_fl_deadband_init_*(ctx, threshold)` | float | value, must be > 0 (0 would mean pass-through, so init rejects it) |
| `elib_fl_deadband_set_*(ctx, value)` | float | value; returns `NOT_INITIALIZED` when not initialized |
| `elib_fl_deadband_update_*(ctx, in)` | float → float | value → same type (the 16/32-bit variants compute the difference in int32/int64, full-scale safe) |
| `elib_fl_deadband_reset_*(ctx)` | ✓ | ✓ |

No window and no warmup concept: the very first update already produces a valid output.

### hyst — Single-Boundary Hysteresis

> One decision boundary `boundary` with three parameters: hysteresis width `hyst` (> 0) and snap-through width `snap` (> hyst, enforced by init).
> A sample climbing toward the boundary is **clamped at boundary** while in `boundary < in ≤ boundary + hyst`; after crossing, a fall-back of `≤ hyst` is **held** (Schmitt hysteresis); a real jump with `|in − out| > snap` **passes straight through**.

| Function | float | Integer five-variant |
|------|-------|-----------|
| `elib_fl_hyst_init_*(ctx, boundary, hyst, snap)` | float | value; `hyst > 0` and `snap > hyst`, violations rejected at init |
| `elib_fl_hyst_set_*(ctx, value)` | float | seed value (HIGH/LOW is decided automatically); returns `NOT_INITIALIZED` when not initialized |
| `elib_fl_hyst_update_*(ctx, in)` | float → float | value → same type (NaN is swallowed and the output is held) |
| `elib_fl_hyst_reset_*(ctx)` | ✓ | ✓ |

**No grading**: one instance per boundary; the application layer derives the level by combining the `above` bits of several instances.

### median — Median Filter

> Collect N samples, sort them, and take the median (for an even window it takes `buf[size/2]` after sorting, i.e. the larger middle element); non-overlapping windows.
> **Note**: sorting happens **in place** in the user buffer, so the buffer contents are left sorted after every output (the next batch overwrites it anyway, usually harmless, but you should know).

| Function | float | Integer five-variant |
|------|-------|-----------|
| `elib_fl_median_init_*(ctx, buf, size)` | float* | value pointer (owned and supplied by the user) |
| `elib_fl_median_update_*(ctx, in)` | float → float | value → same type |
| `elib_fl_median_ready_*(ctx)` | ✓ | ✓ |
| `elib_fl_median_reset_*(ctx)` | ✓ | ✓ |

### avg — Average Filter

> Collect N samples and average them; non-overlapping windows. How the integer variants round: truncation toward zero (C99 integer division semantics).

| Function | float | Integer five-variant |
|------|-------|-----------|
| `elib_fl_avg_init_*(ctx, size)` | ✓ | ✓ (i16/u16 variants require size ≤ 65535, larger values rejected) |
| `elib_fl_avg_update_*(ctx, in)` | float → float | value → same type |
| `elib_fl_avg_ready_*(ctx)` | ✓ | ✓ |
| `elib_fl_avg_reset_*(ctx)` | ✓ | ✓ |

### mavg — Moving Average Filter

> Sliding window, O(1) update, one output per sample; while the window is filling it returns a partial average, and once full it always returns the full-window average. init zeroes the user buffer; reset does not clear the buffer again (the window logic never reads stale contents before it has refilled).

| Function | float | Integer five-variant |
|------|-------|-----------|
| `elib_fl_mavg_init_*(ctx, buf, size)` | float* | value pointer (i16/u16 variants require size ≤ 65535, larger values rejected) |
| `elib_fl_mavg_update_*(ctx, in)` | float → float | value → same type |
| `elib_fl_mavg_ready_*(ctx)` | ✓ | ✓ |
| `elib_fl_mavg_reset_*(ctx)` | ✓ | ✓ |

### v1 → v2 Migration Mapping

| v1 | v2 | Note |
|----|----|------|
| `elib_fl_*_warmup_*` | `elib_fl_*_ready_*` | return value semantics unchanged (0/1); the name now tells the truth |
| `avg/mavg` 32-bit accumulator | 64-bit accumulator | **ctx memory layout change** (the type of the `sum` field); i32/u32 cannot overflow for any sample × any window length |
| out-of-range window length silently UB/wraparound | rejected at init | illegal configurations never reach run time |
| `lpf1_i32` α rounding gap | added `scaled >= 2^n` rejection | aligned with its u32 sibling |
| 6 sub-headers + 5 internal shell headers | single `elib_fl.h` | sub-headers removed, every `.c` includes the umbrella header directly |
| init does not clear ctx | init memsets ctx | a full ctx invariant |
| — | added `_i16`/`_u16` five-variant sets | 32-bit intermediates plus a window-length cap to rule out overflow |
| — | added the `deadband` module | deadband filtering (the mirror of slew) |

### v2 → v3 Migration Mapping

| v2 | v3 | Note |
|----|----|------|
| `elib_fl_clip_*` | `elib_fl_slew_*` | Rename to what it is: a slew-rate limiter. 10 types and 20 functions renamed wholesale, semantics and parameters unchanged; no compatibility aliases |
| — | added the `hyst` module | single-boundary hysteresis (boundary/hyst/snap), seven modules × five variants = 35 sources; this breaking public-API change is recorded in major version `3.0.0` together with the rename |

When the window is not yet full or the context is not initialized, update returns 0 (sentinel semantics; when 0 is also a legal output, the ambiguity is resolved with `ready()`) — the v2 docs pin this behavior down, and no error channel is added.

### Error Codes

| Error code | Description |
|--------|------|
| `ELIB_FL_OK` | Success |
| `ELIB_FL_ERR_INVALID_PARAM` | Invalid parameter (NULL, empty window, deadband alpha, etc.; rejected once at init) |
| `ELIB_FL_ERR_NOT_INITIALIZED` | Not initialized |

## Directory Layout

```
elib-filter/
├── include/
│   └── elib_fl.h                  # single public header (error codes + seven modules × five variants)
├── src/
│   ├── elib_fl_lpf1_core_f.c      # lpf1 five-variant implementations
│   ├── elib_fl_lpf1_core_i16.c
│   ├── elib_fl_lpf1_core_i32.c
│   ├── elib_fl_lpf1_core_u16.c
│   ├── elib_fl_lpf1_core_u32.c
│   ├── elib_fl_slew_core_f.c        # slew five-variant implementations (clip of v2, renamed)
│   ├── elib_fl_slew_core_i16.c
│   ├── elib_fl_slew_core_i32.c
│   ├── elib_fl_slew_core_u16.c
│   ├── elib_fl_slew_core_u32.c
│   ├── elib_fl_deadband_core_f.c    # deadband five-variant implementations
│   ├── elib_fl_deadband_core_i16.c
│   ├── elib_fl_deadband_core_i32.c
│   ├── elib_fl_deadband_core_u16.c
│   ├── elib_fl_deadband_core_u32.c
│   ├── elib_fl_hyst_core_f.c        # hyst five-variant implementations
│   ├── elib_fl_hyst_core_i16.c
│   ├── elib_fl_hyst_core_i32.c
│   ├── elib_fl_hyst_core_u16.c
│   ├── elib_fl_hyst_core_u32.c
│   ├── elib_fl_median_core_f.c    # median five-variant implementations
│   ├── elib_fl_median_core_i16.c
│   ├── elib_fl_median_core_i32.c
│   ├── elib_fl_median_core_u16.c
│   ├── elib_fl_median_core_u32.c
│   ├── elib_fl_avg_core_f.c       # avg five-variant implementations
│   ├── elib_fl_avg_core_i16.c
│   ├── elib_fl_avg_core_i32.c
│   ├── elib_fl_avg_core_u16.c
│   ├── elib_fl_avg_core_u32.c
│   ├── elib_fl_mavg_core_f.c      # mavg five-variant implementations
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

## Building

No build system, a single compile command (arm-none-eabi-gcc or any C99 compiler):

```bash
arm-none-eabi-gcc -std=c99 -Wall -Wextra -Werror -Iinclude -c src/*.c
```

Each translation unit includes `elib_fl.h` directly (`-Iinclude`, no relative path injection); a multi-file project just links any combination of the `.o` files.

## License

MIT License
