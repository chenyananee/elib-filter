/* elib_fl.h - Embedded filter library, single umbrella header.
 *
 * Six independent filters, each in five numeric flavours
 * (explicit `_f` / `_i16` / `_i32` / `_u16` / `_u32` suffixes, pure
 * C99, no macro dispatch):
 *   lpf1     first-order low-pass IIR (alpha / time-constant /
 *            cutoff-frequency setups; Qn fixed-point alpha for ints)
 *   clip     rate-of-change limiter (rejects steps beyond threshold,
 *            holds last output)
 *   deadband dead-zone filter (ignores changes within threshold --
 *            the mirror image of clip: small held, large followed)
 *   median   non-overlapping batch median (one output every N samples)
 *   avg      non-overlapping batch mean (one output every N samples)
 *   mavg     sliding-window mean (one output every sample, O(1) update)
 *
 * Rules of the game:
 *  - zero dynamic memory: every context is user allocated and self
 *    contained; no globals anywhere in the library
 *  - configuration is validated ONCE at init and rejected there
 *    (NULL ctx, empty windows, out-of-range alphas) -- the library
 *    never carries an illegal configuration into runtime
 *  - integer accumulators are 64-bit in the int32/uint32 flavours:
 *    any value over any window length without overflow. The int16/
 *    uint16 flavours use 32-bit intermediates, which are bound-
 *    provably safe for 16-bit data (window sums capped by init:
 *    size <= 65535 makes |sum| <= 2^31 / 2^32 always hold)
 *  - update() / ready() return VALUES, not errors; a zero return is
 *    a documented sentinel during warmup and on uninitialized ctx
 *    (use ready() to disambiguate warmup -- never read the raw 0)
 *
 * Version: 2.0.0
 */
#ifndef ELIB_FL_H
#define ELIB_FL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ELIB_FL_VERSION "2.0.0"

/* ---- error codes (shared by all filters) ---- */

typedef enum {
    ELIB_FL_OK = 0,
    ELIB_FL_ERR_INVALID_PARAM,   /* NULL ctx / empty window / dead alpha */
    ELIB_FL_ERR_NOT_INITIALIZED  /* API called before a valid init */
} elib_fl_err_t;

/* ================================================================
 * lpf1 -- first-order low-pass filter
 *
 *   y[n] = y[n-1] + alpha * (x[n] - y[n-1])
 *
 * float flavour: alpha in (0, 1).
 * int flavours: alpha is Qn fixed point (n = fractional bits,
 * 0 < alpha < 2^n); update() works on int64 intermediates, the full
 * int32/uint32 input range is safe.
 * ================================================================ */

typedef struct {
    float   out;
    float   alpha;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_lpf1_ctx_f_t;

/**
 * Initialize with a direct alpha
 * @param alpha Filter factor, must be in (0.0, 1.0)
 */
elib_fl_err_t elib_fl_lpf1_init_f(elib_fl_lpf1_ctx_f_t *ctx, float alpha);

/**
 * Initialize from time constant: alpha = dt / (tau + dt)
 * @param tau Time constant in seconds, > 0
 * @param dt  Sample period in seconds, > 0
 */
elib_fl_err_t elib_fl_lpf1_init_tau_f(elib_fl_lpf1_ctx_f_t *ctx, float tau, float dt);

/**
 * Initialize from cutoff frequency: alpha = (1/fs) / (rc + 1/fs),
 * rc = 1 / (2*pi*fc)
 * @param fc Cutoff frequency in Hz, > 0
 * @param fs Sample frequency in Hz, > 0
 */
elib_fl_err_t elib_fl_lpf1_init_fc_f(elib_fl_lpf1_ctx_f_t *ctx, float fc, float fs);

/**
 * Process one sample
 * @return Filtered output; 0.0f on NULL/uninitialized ctx
 */
float elib_fl_lpf1_update_f(elib_fl_lpf1_ctx_f_t *ctx, float in);

/** Clear the internal state (alpha kept) */
void elib_fl_lpf1_reset_f(elib_fl_lpf1_ctx_f_t *ctx);

typedef struct {
    int32_t out;
    int32_t alpha;
    uint8_t n;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_lpf1_ctx_i32_t;

/**
 * Initialize with a direct Qn alpha
 * @param alpha Qn fixed-point alpha, 0 < alpha < 2^n
 * @param n     Fractional bit count, 1..31
 */
elib_fl_err_t elib_fl_lpf1_init_i32(elib_fl_lpf1_ctx_i32_t *ctx, int32_t alpha, uint8_t n);

/** As init_i32: tau/dt convert to alpha, then quantize to Qn */
elib_fl_err_t elib_fl_lpf1_init_tau_i32(elib_fl_lpf1_ctx_i32_t *ctx, float tau, float dt, uint8_t n);

/** As init_i32: fc/fs convert to alpha, then quantize to Qn */
elib_fl_err_t elib_fl_lpf1_init_fc_i32(elib_fl_lpf1_ctx_i32_t *ctx, float fc, float fs, uint8_t n);

/** Process one sample; 0 on NULL/uninitialized ctx */
int32_t elib_fl_lpf1_update_i32(elib_fl_lpf1_ctx_i32_t *ctx, int32_t in);

void elib_fl_lpf1_reset_i32(elib_fl_lpf1_ctx_i32_t *ctx);

typedef struct {
    uint32_t out;
    uint32_t alpha;
    uint8_t  n;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_lpf1_ctx_u32_t;

/** Initialize with a direct Qn alpha, 0 < alpha < 2^n, n in 1..31 */
elib_fl_err_t elib_fl_lpf1_init_u32(elib_fl_lpf1_ctx_u32_t *ctx, uint32_t alpha, uint8_t n);

/** As init_u32: tau/dt convert to alpha, then quantize to Qn */
elib_fl_err_t elib_fl_lpf1_init_tau_u32(elib_fl_lpf1_ctx_u32_t *ctx, float tau, float dt, uint8_t n);

/** As init_u32: fc/fs convert to alpha, then quantize to Qn */
elib_fl_err_t elib_fl_lpf1_init_fc_u32(elib_fl_lpf1_ctx_u32_t *ctx, float fc, float fs, uint8_t n);

/** Process one sample; 0 on NULL/uninitialized ctx */
uint32_t elib_fl_lpf1_update_u32(elib_fl_lpf1_ctx_u32_t *ctx, uint32_t in);

void elib_fl_lpf1_reset_u32(elib_fl_lpf1_ctx_u32_t *ctx);

/* -- int16 flavour: Qn alpha with n in 1..15; int32 intermediates
 * are bound-provably safe (|alpha| <= 2^15-1, |diff| <= 2^16-1, so
 * |alpha * diff| < 2^31 without any 64-bit step) -- */

typedef struct {
    int16_t out;
    int16_t alpha;
    uint8_t n;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_lpf1_ctx_i16_t;

/**
 * Initialize with a direct Qn alpha
 * @param alpha Qn fixed-point alpha, 0 < alpha < 2^n <= 2^15
 * @param n     Fractional bit count, 1..15
 */
elib_fl_err_t elib_fl_lpf1_init_i16(elib_fl_lpf1_ctx_i16_t *ctx, int16_t alpha, uint8_t n);

/** As init_i16: tau/dt convert to alpha, then quantize to Qn */
elib_fl_err_t elib_fl_lpf1_init_tau_i16(elib_fl_lpf1_ctx_i16_t *ctx, float tau, float dt, uint8_t n);

/** As init_i16: fc/fs convert to alpha, then quantize to Qn */
elib_fl_err_t elib_fl_lpf1_init_fc_i16(elib_fl_lpf1_ctx_i16_t *ctx, float fc, float fs, uint8_t n);

/** Process one sample (int32 intermediates); 0 on NULL/uninitialized ctx */
int16_t elib_fl_lpf1_update_i16(elib_fl_lpf1_ctx_i16_t *ctx, int16_t in);

void elib_fl_lpf1_reset_i16(elib_fl_lpf1_ctx_i16_t *ctx);

/* -- uint16 flavour: Qn alpha with n in 1..15 (u32's n <= 31 rule is
 * width-1; the same width-1 cap keeps alpha below 2^15 so a near-
 * pass-through alpha can never be stored) -- */

typedef struct {
    uint16_t out;
    uint16_t alpha;
    uint8_t  n;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_lpf1_ctx_u16_t;

/** Initialize with a direct Qn alpha, 0 < alpha < 2^n <= 2^15, n in 1..15 */
elib_fl_err_t elib_fl_lpf1_init_u16(elib_fl_lpf1_ctx_u16_t *ctx, uint16_t alpha, uint8_t n);

/** As init_u16: tau/dt convert to alpha, then quantize to Qn */
elib_fl_err_t elib_fl_lpf1_init_tau_u16(elib_fl_lpf1_ctx_u16_t *ctx, float tau, float dt, uint8_t n);

/** As init_u16: fc/fs convert to alpha, then quantize to Qn */
elib_fl_err_t elib_fl_lpf1_init_fc_u16(elib_fl_lpf1_ctx_u16_t *ctx, float fc, float fs, uint8_t n);

/** Process one sample (int32 diff, int32 product); 0 on NULL/uninitialized ctx */
uint16_t elib_fl_lpf1_update_u16(elib_fl_lpf1_ctx_u16_t *ctx, uint16_t in);

void elib_fl_lpf1_reset_u16(elib_fl_lpf1_ctx_u16_t *ctx);

/* ================================================================
 * clip -- rate-of-change limiter
 *
 *   |x[n] - y[n-1]| <= threshold -> y[n] = x[n]
 *   otherwise                    -> y[n] = y[n-1]   (step rejected)
 *
 * Note the semantics: this rejects LARGE STEPS, it does not clamp
 * amplitudes. The name is historical.
 * ================================================================ */

typedef struct {
    float out;
    float threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_clip_ctx_f_t;

/** Initialize with a threshold, must be > 0 */
elib_fl_err_t elib_fl_clip_init_f(elib_fl_clip_ctx_f_t *ctx, float threshold);

/**
 * Set the current output state (initial value for the limiter)
 * @return ELIB_FL_ERR_NOT_INITIALIZED on uninitialized ctx
 */
elib_fl_err_t elib_fl_clip_set_f(elib_fl_clip_ctx_f_t *ctx, float value);

/** Process one sample; 0.0f on NULL/uninitialized ctx */
float elib_fl_clip_update_f(elib_fl_clip_ctx_f_t *ctx, float in);

/** Clear the output state (threshold kept) */
void elib_fl_clip_reset_f(elib_fl_clip_ctx_f_t *ctx);

typedef struct {
    int32_t out;
    int32_t threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_clip_ctx_i32_t;

/** Initialize with a threshold, must be > 0 */
elib_fl_err_t elib_fl_clip_init_i32(elib_fl_clip_ctx_i32_t *ctx, int32_t threshold);

/** Set the current output state; NOT_INITIALIZED on uninitialized ctx */
elib_fl_err_t elib_fl_clip_set_i32(elib_fl_clip_ctx_i32_t *ctx, int32_t value);

/** Process one sample (int64 diff internally, full range safe); 0 on
 * NULL/uninitialized ctx */
int32_t elib_fl_clip_update_i32(elib_fl_clip_ctx_i32_t *ctx, int32_t in);

void elib_fl_clip_reset_i32(elib_fl_clip_ctx_i32_t *ctx);

typedef struct {
    uint32_t out;
    uint32_t threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_clip_ctx_u32_t;

/** Initialize with a threshold, must be > 0 */
elib_fl_err_t elib_fl_clip_init_u32(elib_fl_clip_ctx_u32_t *ctx, uint32_t threshold);

/** Set the current output state; NOT_INITIALIZED on uninitialized ctx */
elib_fl_err_t elib_fl_clip_set_u32(elib_fl_clip_ctx_u32_t *ctx, uint32_t value);

/** Process one sample (int64 diff internally, full range safe); 0 on
 * NULL/uninitialized ctx */
uint32_t elib_fl_clip_update_u32(elib_fl_clip_ctx_u32_t *ctx, uint32_t in);

void elib_fl_clip_reset_u32(elib_fl_clip_ctx_u32_t *ctx);

/* -- int16 flavour: the diff fits int32 with room to spare
 * (|diff| <= 2^16-1), so no 64-bit step is needed -- */

typedef struct {
    int16_t out;
    int16_t threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_clip_ctx_i16_t;

/** Initialize with a threshold, must be > 0 */
elib_fl_err_t elib_fl_clip_init_i16(elib_fl_clip_ctx_i16_t *ctx, int16_t threshold);

/** Set the current output state; NOT_INITIALIZED on uninitialized ctx */
elib_fl_err_t elib_fl_clip_set_i16(elib_fl_clip_ctx_i16_t *ctx, int16_t value);

/** Process one sample (int32 diff internally); 0 on NULL/uninitialized ctx */
int16_t elib_fl_clip_update_i16(elib_fl_clip_ctx_i16_t *ctx, int16_t in);

void elib_fl_clip_reset_i16(elib_fl_clip_ctx_i16_t *ctx);

typedef struct {
    uint16_t out;
    uint16_t threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_clip_ctx_u16_t;

/** Initialize with a threshold, must be > 0 */
elib_fl_err_t elib_fl_clip_init_u16(elib_fl_clip_ctx_u16_t *ctx, uint16_t threshold);

/** Set the current output state; NOT_INITIALIZED on uninitialized ctx */
elib_fl_err_t elib_fl_clip_set_u16(elib_fl_clip_ctx_u16_t *ctx, uint16_t value);

/** Process one sample (int32 diff internally); 0 on NULL/uninitialized ctx */
uint16_t elib_fl_clip_update_u16(elib_fl_clip_ctx_u16_t *ctx, uint16_t in);

void elib_fl_clip_reset_u16(elib_fl_clip_ctx_u16_t *ctx);

/* ================================================================
 * deadband -- dead-zone filter
 *
 *   |x[n] - y[n-1]| <= threshold -> y[n] = y[n-1]   (held: ignored)
 *   |x[n] - y[n-1]| >  threshold -> y[n] = x[n]     (followed)
 *
 * The mirror image of clip: small changes are IGNORED (anti-jitter,
 * report-by-exception, static-display anti-dither), large ones pass
 * through unchanged. threshold 0 would follow every nonzero change
 * (a pure pass-through) and is rejected at init. No window, no
 * warmup: valid output from the very first update.
 * ================================================================ */

typedef struct {
    float out;
    float threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_deadband_ctx_f_t;

/** Initialize with a dead-zone width, must be > 0 */
elib_fl_err_t elib_fl_deadband_init_f(elib_fl_deadband_ctx_f_t *ctx, float threshold);

/**
 * Set the current output state (the reference the dead zone is
 * measured against)
 * @return ELIB_FL_ERR_NOT_INITIALIZED on uninitialized ctx
 */
elib_fl_err_t elib_fl_deadband_set_f(elib_fl_deadband_ctx_f_t *ctx, float value);

/** Process one sample; 0.0f on NULL/uninitialized ctx */
float elib_fl_deadband_update_f(elib_fl_deadband_ctx_f_t *ctx, float in);

/** Clear the output state (threshold kept) */
void elib_fl_deadband_reset_f(elib_fl_deadband_ctx_f_t *ctx);

/* -- int16 flavour: int32 diff, bound-provably safe -- */

typedef struct {
    int16_t out;
    int16_t threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_deadband_ctx_i16_t;

/** Initialize with a dead-zone width, must be > 0 */
elib_fl_err_t elib_fl_deadband_init_i16(elib_fl_deadband_ctx_i16_t *ctx, int16_t threshold);

/** Set the output state; NOT_INITIALIZED on uninitialized ctx */
elib_fl_err_t elib_fl_deadband_set_i16(elib_fl_deadband_ctx_i16_t *ctx, int16_t value);

/** Process one sample (int32 diff internally); 0 on NULL/uninitialized ctx */
int16_t elib_fl_deadband_update_i16(elib_fl_deadband_ctx_i16_t *ctx, int16_t in);

void elib_fl_deadband_reset_i16(elib_fl_deadband_ctx_i16_t *ctx);

typedef struct {
    int32_t out;
    int32_t threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_deadband_ctx_i32_t;

/** Initialize with a dead-zone width, must be > 0 */
elib_fl_err_t elib_fl_deadband_init_i32(elib_fl_deadband_ctx_i32_t *ctx, int32_t threshold);

/** Set the output state; NOT_INITIALIZED on uninitialized ctx */
elib_fl_err_t elib_fl_deadband_set_i32(elib_fl_deadband_ctx_i32_t *ctx, int32_t value);

/** Process one sample (int64 diff internally, full range safe); 0 on
 * NULL/uninitialized ctx */
int32_t elib_fl_deadband_update_i32(elib_fl_deadband_ctx_i32_t *ctx, int32_t in);

void elib_fl_deadband_reset_i32(elib_fl_deadband_ctx_i32_t *ctx);

/* -- uint16 flavour: int32 diff, bound-provably safe -- */

typedef struct {
    uint16_t out;
    uint16_t threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_deadband_ctx_u16_t;

/** Initialize with a dead-zone width, must be > 0 */
elib_fl_err_t elib_fl_deadband_init_u16(elib_fl_deadband_ctx_u16_t *ctx, uint16_t threshold);

/** Set the output state; NOT_INITIALIZED on uninitialized ctx */
elib_fl_err_t elib_fl_deadband_set_u16(elib_fl_deadband_ctx_u16_t *ctx, uint16_t value);

/** Process one sample (int32 diff internally); 0 on NULL/uninitialized ctx */
uint16_t elib_fl_deadband_update_u16(elib_fl_deadband_ctx_u16_t *ctx, uint16_t in);

void elib_fl_deadband_reset_u16(elib_fl_deadband_ctx_u16_t *ctx);

typedef struct {
    uint32_t out;
    uint32_t threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_deadband_ctx_u32_t;

/** Initialize with a dead-zone width, must be > 0 */
elib_fl_err_t elib_fl_deadband_init_u32(elib_fl_deadband_ctx_u32_t *ctx, uint32_t threshold);

/** Set the output state; NOT_INITIALIZED on uninitialized ctx */
elib_fl_err_t elib_fl_deadband_set_u32(elib_fl_deadband_ctx_u32_t *ctx, uint32_t value);

/** Process one sample (int64 diff internally, full range safe); 0 on
 * NULL/uninitialized ctx */
uint32_t elib_fl_deadband_update_u32(elib_fl_deadband_ctx_u32_t *ctx, uint32_t in);

void elib_fl_deadband_reset_u32(elib_fl_deadband_ctx_u32_t *ctx);

/* ================================================================
 * median -- non-overlapping batch median
 *
 * Collects a window of `size` samples, then returns their median
 * (larger middle element, buf[size/2] after sorting). The user-owned
 * buffer is SORTED IN PLACE on every output: its contents are
 * destroyed at batch boundaries (they would be overwritten anyway).
 * One valid output every `size` updates; 0 is the warmup sentinel.
 * ================================================================ */

typedef struct {
    float    *buf;
    uint32_t  size;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
        uint8_t ready       : 1;
    } bit_flags;
} elib_fl_median_ctx_f_t;

/** Bind a user-owned buffer of `size` elements; size must be > 0 */
elib_fl_err_t elib_fl_median_init_f(elib_fl_median_ctx_f_t *ctx, float *buf, uint32_t size);

/**
 * Push one sample
 * @return 0 (warmup sentinel) until the window is full; the batch
 *         median once every `size` updates
 */
float elib_fl_median_update_f(elib_fl_median_ctx_f_t *ctx, float in);

/**
 * @return 1 if the LAST update produced a valid median, 0 otherwise
 * (also 0 on NULL/uninitialized ctx)
 */
uint32_t elib_fl_median_ready_f(const elib_fl_median_ctx_f_t *ctx);

/** Clear the window counter; ready goes back to 0, buffer untouched */
void elib_fl_median_reset_f(elib_fl_median_ctx_f_t *ctx);

typedef struct {
    int32_t  *buf;
    uint32_t  size;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
        uint8_t ready       : 1;
    } bit_flags;
} elib_fl_median_ctx_i32_t;

/** Bind a user-owned buffer of `size` elements; size must be > 0 */
elib_fl_err_t elib_fl_median_init_i32(elib_fl_median_ctx_i32_t *ctx, int32_t *buf, uint32_t size);

/** Push one sample; 0 = warmup sentinel, see float flavour */
int32_t elib_fl_median_update_i32(elib_fl_median_ctx_i32_t *ctx, int32_t in);

/** See float flavour */
uint32_t elib_fl_median_ready_i32(const elib_fl_median_ctx_i32_t *ctx);

void elib_fl_median_reset_i32(elib_fl_median_ctx_i32_t *ctx);

typedef struct {
    uint32_t *buf;
    uint32_t  size;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
        uint8_t ready       : 1;
    } bit_flags;
} elib_fl_median_ctx_u32_t;

/** Bind a user-owned buffer of `size` elements; size must be > 0 */
elib_fl_err_t elib_fl_median_init_u32(elib_fl_median_ctx_u32_t *ctx, uint32_t *buf, uint32_t size);

/** Push one sample; 0 = warmup sentinel, see float flavour */
uint32_t elib_fl_median_update_u32(elib_fl_median_ctx_u32_t *ctx, uint32_t in);

/** See float flavour */
uint32_t elib_fl_median_ready_u32(const elib_fl_median_ctx_u32_t *ctx);

void elib_fl_median_reset_u32(elib_fl_median_ctx_u32_t *ctx);

/* -- int16 flavour -- */

typedef struct {
    int16_t  *buf;
    uint32_t  size;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
        uint8_t ready       : 1;
    } bit_flags;
} elib_fl_median_ctx_i16_t;

/** Bind a user-owned buffer of `size` elements; size must be > 0 */
elib_fl_err_t elib_fl_median_init_i16(elib_fl_median_ctx_i16_t *ctx, int16_t *buf, uint32_t size);

/** Push one sample; 0 = warmup sentinel, see float flavour */
int16_t elib_fl_median_update_i16(elib_fl_median_ctx_i16_t *ctx, int16_t in);

/** See float flavour */
uint32_t elib_fl_median_ready_i16(const elib_fl_median_ctx_i16_t *ctx);

void elib_fl_median_reset_i16(elib_fl_median_ctx_i16_t *ctx);

typedef struct {
    uint16_t *buf;
    uint32_t  size;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
        uint8_t ready       : 1;
    } bit_flags;
} elib_fl_median_ctx_u16_t;

/** Bind a user-owned buffer of `size` elements; size must be > 0 */
elib_fl_err_t elib_fl_median_init_u16(elib_fl_median_ctx_u16_t *ctx, uint16_t *buf, uint32_t size);

/** Push one sample; 0 = warmup sentinel, see float flavour */
uint16_t elib_fl_median_update_u16(elib_fl_median_ctx_u16_t *ctx, uint16_t in);

/** See float flavour */
uint32_t elib_fl_median_ready_u16(const elib_fl_median_ctx_u16_t *ctx);

void elib_fl_median_reset_u16(elib_fl_median_ctx_u16_t *ctx);

/* ================================================================
 * avg -- non-overlapping batch mean
 *
 * Accumulates a window of `size` samples (64-bit accumulator: any
 * int32/uint32 values, any window length), returns sum/size (trunc
 * for ints) once every `size` updates, then restarts. 0 is the
 * warmup sentinel -- a genuine zero mean is indistinguishable, use
 * ready().
 * ================================================================ */

typedef struct {
    float     sum;      /* running sum of the current window */
    uint32_t  size;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
        uint8_t ready       : 1;
    } bit_flags;
} elib_fl_avg_ctx_f_t;

/** Set the window size, must be > 0 */
elib_fl_err_t elib_fl_avg_init_f(elib_fl_avg_ctx_f_t *ctx, uint32_t size);

/** Push one sample; 0 = warmup sentinel, mean once every `size` */
float elib_fl_avg_update_f(elib_fl_avg_ctx_f_t *ctx, float in);

/** 1 if the LAST update produced a valid mean, else 0 */
uint32_t elib_fl_avg_ready_f(const elib_fl_avg_ctx_f_t *ctx);

/** Clear the accumulator and window counter */
void elib_fl_avg_reset_f(elib_fl_avg_ctx_f_t *ctx);

typedef struct {
    int64_t   sum;      /* 64-bit: overflow-free accumulation */
    uint32_t  size;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
        uint8_t ready       : 1;
    } bit_flags;
} elib_fl_avg_ctx_i32_t;

/** Set the window size, must be > 0 */
elib_fl_err_t elib_fl_avg_init_i32(elib_fl_avg_ctx_i32_t *ctx, uint32_t size);

/** Push one sample; result is sum/size truncated toward zero */
int32_t elib_fl_avg_update_i32(elib_fl_avg_ctx_i32_t *ctx, int32_t in);

/** 1 if the LAST update produced a valid mean, else 0 */
uint32_t elib_fl_avg_ready_i32(const elib_fl_avg_ctx_i32_t *ctx);

void elib_fl_avg_reset_i32(elib_fl_avg_ctx_i32_t *ctx);

typedef struct {
    uint64_t  sum;      /* 64-bit: overflow-free accumulation */
    uint32_t  size;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
        uint8_t ready       : 1;
    } bit_flags;
} elib_fl_avg_ctx_u32_t;

/** Set the window size, must be > 0 */
elib_fl_err_t elib_fl_avg_init_u32(elib_fl_avg_ctx_u32_t *ctx, uint32_t size);

/** Push one sample; result is sum/size truncated */
uint32_t elib_fl_avg_update_u32(elib_fl_avg_ctx_u32_t *ctx, uint32_t in);

/** 1 if the LAST update produced a valid mean, else 0 */
uint32_t elib_fl_avg_ready_u32(const elib_fl_avg_ctx_u32_t *ctx);

void elib_fl_avg_reset_u32(elib_fl_avg_ctx_u32_t *ctx);

/* -- int16 flavour: 32-bit accumulator, bound-provably overflow-free
 * because init REJECTS size > 65535 (|sum| <= 65535 * 2^15 < 2^31) -- */

typedef struct {
    int32_t   sum;      /* running sum of the current window */
    uint32_t  size;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
        uint8_t ready       : 1;
    } bit_flags;
} elib_fl_avg_ctx_i16_t;

/** Set the window size, must be > 0 and <= 65535 */
elib_fl_err_t elib_fl_avg_init_i16(elib_fl_avg_ctx_i16_t *ctx, uint32_t size);

/** Push one sample; 0 = warmup sentinel, mean once every `size` */
int16_t elib_fl_avg_update_i16(elib_fl_avg_ctx_i16_t *ctx, int16_t in);

/** 1 if the LAST update produced a valid mean, else 0 */
uint32_t elib_fl_avg_ready_i16(const elib_fl_avg_ctx_i16_t *ctx);

/** Clear the accumulator and window counter */
void elib_fl_avg_reset_i16(elib_fl_avg_ctx_i16_t *ctx);

/* -- uint16 flavour: 32-bit accumulator safe for size <= 65535
 * (sum <= 65535 * 65535 < 2^32); larger sizes are rejected at init -- */

typedef struct {
    uint32_t  sum;      /* running sum of the current window */
    uint32_t  size;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
        uint8_t ready       : 1;
    } bit_flags;
} elib_fl_avg_ctx_u16_t;

/** Set the window size, must be > 0 and <= 65535 */
elib_fl_err_t elib_fl_avg_init_u16(elib_fl_avg_ctx_u16_t *ctx, uint32_t size);

/** Push one sample; result is sum/size truncated */
uint16_t elib_fl_avg_update_u16(elib_fl_avg_ctx_u16_t *ctx, uint16_t in);

/** 1 if the LAST update produced a valid mean, else 0 */
uint32_t elib_fl_avg_ready_u16(const elib_fl_avg_ctx_u16_t *ctx);

void elib_fl_avg_reset_u16(elib_fl_avg_ctx_u16_t *ctx);

/* ================================================================
 * mavg -- sliding-window mean
 *
 * O(1) sliding average over the last `size` samples (ring buffer +
 * 64-bit accumulator). EVERY update returns the current window mean
 * (a partial mean while filling; becomes the full-window mean once
 * filled, where it stays until reset).
 * ================================================================ */

typedef struct {
    float    *buf;
    float     sum;
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_mavg_ctx_f_t;

/**
 * Bind a user-owned buffer of `size` elements; the buffer is zeroed
 * by init. size must be > 0.
 */
elib_fl_err_t elib_fl_mavg_init_f(elib_fl_mavg_ctx_f_t *ctx, float *buf, uint32_t size);

/** Push one sample; returns the current (possibly partial) mean */
float elib_fl_mavg_update_f(elib_fl_mavg_ctx_f_t *ctx, float in);

/** 1 once the window has filled (stays 1 until reset), else 0 */
uint32_t elib_fl_mavg_ready_f(const elib_fl_mavg_ctx_f_t *ctx);

/** Clear the state; the buffer is NOT re-zeroed (contents stay, but
 * the window logic discards them until refilled) */
void elib_fl_mavg_reset_f(elib_fl_mavg_ctx_f_t *ctx);

typedef struct {
    int32_t  *buf;
    int64_t   sum;      /* 64-bit: overflow-free accumulation */
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_mavg_ctx_i32_t;

/** Bind a user-owned buffer (zeroed by init); size must be > 0 */
elib_fl_err_t elib_fl_mavg_init_i32(elib_fl_mavg_ctx_i32_t *ctx, int32_t *buf, uint32_t size);

/** Push one sample; returns sum/count truncated toward zero */
int32_t elib_fl_mavg_update_i32(elib_fl_mavg_ctx_i32_t *ctx, int32_t in);

/** 1 once the window has filled, else 0 */
uint32_t elib_fl_mavg_ready_i32(const elib_fl_mavg_ctx_i32_t *ctx);

void elib_fl_mavg_reset_i32(elib_fl_mavg_ctx_i32_t *ctx);

typedef struct {
    uint32_t *buf;
    uint64_t  sum;      /* 64-bit: overflow-free accumulation */
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_mavg_ctx_u32_t;

/** Bind a user-owned buffer (zeroed by init); size must be > 0 */
elib_fl_err_t elib_fl_mavg_init_u32(elib_fl_mavg_ctx_u32_t *ctx, uint32_t *buf, uint32_t size);

/** Push one sample; returns sum/count truncated */
uint32_t elib_fl_mavg_update_u32(elib_fl_mavg_ctx_u32_t *ctx, uint32_t in);

/** 1 once the window has filled, else 0 */
uint32_t elib_fl_mavg_ready_u32(const elib_fl_mavg_ctx_u32_t *ctx);

void elib_fl_mavg_reset_u32(elib_fl_mavg_ctx_u32_t *ctx);

/* -- int16 flavour: 32-bit accumulator safe for size <= 65535
 * (|sum| <= 65535 * 2^15 < 2^31); larger sizes are rejected at init -- */

typedef struct {
    int16_t  *buf;
    int32_t   sum;
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_mavg_ctx_i16_t;

/**
 * Bind a user-owned buffer (zeroed by init); size must be > 0 and
 * <= 65535.
 */
elib_fl_err_t elib_fl_mavg_init_i16(elib_fl_mavg_ctx_i16_t *ctx, int16_t *buf, uint32_t size);

/** Push one sample; returns sum/count truncated toward zero */
int16_t elib_fl_mavg_update_i16(elib_fl_mavg_ctx_i16_t *ctx, int16_t in);

/** 1 once the window has filled, else 0 */
uint32_t elib_fl_mavg_ready_i16(const elib_fl_mavg_ctx_i16_t *ctx);

void elib_fl_mavg_reset_i16(elib_fl_mavg_ctx_i16_t *ctx);

/* -- uint16 flavour: 32-bit accumulator safe for size <= 65535
 * (sum <= 65535 * 65535 < 2^32); larger sizes are rejected at init -- */

typedef struct {
    uint16_t *buf;
    uint32_t  sum;
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_mavg_ctx_u16_t;

/**
 * Bind a user-owned buffer (zeroed by init); size must be > 0 and
 * <= 65535.
 */
elib_fl_err_t elib_fl_mavg_init_u16(elib_fl_mavg_ctx_u16_t *ctx, uint16_t *buf, uint32_t size);

/** Push one sample; returns sum/count truncated */
uint16_t elib_fl_mavg_update_u16(elib_fl_mavg_ctx_u16_t *ctx, uint16_t in);

/** 1 once the window has filled, else 0 */
uint32_t elib_fl_mavg_ready_u16(const elib_fl_mavg_ctx_u16_t *ctx);

void elib_fl_mavg_reset_u16(elib_fl_mavg_ctx_u16_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ELIB_FL_H */
