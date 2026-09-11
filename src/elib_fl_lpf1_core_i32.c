/* elib_fl_lpf1_core_i32.c - First-Order Low-Pass Filter (Int32, Qn alpha) */

#include "elib_fl_lpf1_core.h"

#define ELIB_FL_LPF1_PI 3.14159265358979323846f
#define ELIB_FL_LPF1_I32_MAX 2147483647LL

/* (alpha * diff) >> n without overflowing int64: split diff into high/low
   parts so each product stays well below 2^63. */
static int64_t lpf1_i32_mulshift(int64_t diff, int32_t alpha, uint8_t n)
{
    int64_t hi = diff >> n;
    int64_t lo = diff - (hi << n);
    return hi * (int64_t)alpha + (((int64_t)alpha * lo) >> n);
}

elib_fl_err_t elib_fl_lpf1_init_i32(elib_fl_lpf1_ctx_i32_t *ctx, int32_t alpha, uint8_t n)
{
    if (ctx == NULL || n == 0 || n > 31 || alpha <= 0 || (uint32_t)alpha >= (1U << n)) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->alpha = alpha;
    ctx->n = n;
    ctx->out = 0;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

static elib_fl_err_t lpf1_i32_set_alpha(elib_fl_lpf1_ctx_i32_t *ctx, float alpha_f, uint8_t n)
{
    int64_t scaled = (int64_t)((double)alpha_f * (double)(1U << n) + 0.5);
    if (scaled <= 0 || scaled > ELIB_FL_LPF1_I32_MAX) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->alpha = (int32_t)scaled;
    ctx->n = n;
    ctx->out = 0;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_lpf1_init_tau_i32(elib_fl_lpf1_ctx_i32_t *ctx, float tau, float dt, uint8_t n)
{
    if (ctx == NULL || tau <= 0.0f || dt <= 0.0f || n == 0 || n > 31) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    float alpha_f = dt / (tau + dt);
    if (alpha_f <= 0.0f || alpha_f >= 1.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    return lpf1_i32_set_alpha(ctx, alpha_f, n);
}

elib_fl_err_t elib_fl_lpf1_init_fc_i32(elib_fl_lpf1_ctx_i32_t *ctx, float fc, float fs, uint8_t n)
{
    if (ctx == NULL || fc <= 0.0f || fs <= 0.0f || n == 0 || n > 31) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    float rc = 1.0f / (2.0f * ELIB_FL_LPF1_PI * fc);
    float alpha_f = (1.0f / fs) / (rc + (1.0f / fs));
    if (alpha_f <= 0.0f || alpha_f >= 1.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    return lpf1_i32_set_alpha(ctx, alpha_f, n);
}

int32_t elib_fl_lpf1_update_i32(elib_fl_lpf1_ctx_i32_t *ctx, int32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    int64_t diff = (int64_t)in - (int64_t)ctx->out;
    ctx->out = (int32_t)((int64_t)ctx->out + lpf1_i32_mulshift(diff, ctx->alpha, ctx->n));

    return ctx->out;
}

void elib_fl_lpf1_reset_i32(elib_fl_lpf1_ctx_i32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->out = 0;
}
