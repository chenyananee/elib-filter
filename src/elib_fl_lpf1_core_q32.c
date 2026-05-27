/* elib_fl_lpf1_core_q32.c - First-Order Low-Pass Filter (Q32) */

#include "elib_fl_lpf1_core.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int32_t q32_mul(int32_t a, int32_t b, uint8_t n)
{
    return (int32_t)(((int64_t)a * b) >> n);
}

elib_fl_err_t elib_fl_lpf1_init_q32(elib_fl_lpf1_ctx_q32_t *ctx, int32_t alpha, uint8_t n)
{
    if (ctx == NULL || n == 0 || n > 31 || alpha <= 0 || alpha >= (int32_t)(1U << n)) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->alpha = alpha;
    ctx->n = n;
    ctx->out = 0;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_lpf1_init_tau_q32(elib_fl_lpf1_ctx_q32_t *ctx, float tau, float dt, uint8_t n)
{
    if (ctx == NULL || tau <= 0.0f || dt <= 0.0f || n == 0 || n > 31) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    float alpha_f = dt / (tau + dt);
    int32_t alpha = (int32_t)(alpha_f * (float)(1 << n) + 0.5f);

    ctx->alpha = alpha;
    ctx->n = n;
    ctx->out = 0;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_lpf1_init_fc_q32(elib_fl_lpf1_ctx_q32_t *ctx, float fc, float fs, uint8_t n)
{
    if (ctx == NULL || fc <= 0.0f || fs <= 0.0f || n == 0 || n > 31) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    float rc = 1.0f / (2.0f * (float)M_PI * fc);
    float alpha_f = (1.0f / fs) / (rc + (1.0f / fs));
    int32_t alpha = (int32_t)(alpha_f * (float)(1 << n) + 0.5f);

    ctx->alpha = alpha;
    ctx->n = n;
    ctx->out = 0;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

int32_t elib_fl_lpf1_update_q32(elib_fl_lpf1_ctx_q32_t *ctx, int32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    int32_t diff = in - ctx->out;
    ctx->out = ctx->out + q32_mul(ctx->alpha, diff, ctx->n);

    return ctx->out;
}

int32_t elib_fl_lpf1_oneshot_q32(int32_t alpha, int32_t in, int32_t prev, uint8_t n)
{
    int32_t diff = in - prev;
    return prev + q32_mul(alpha, diff, n);
}

void elib_fl_lpf1_reset_q32(elib_fl_lpf1_ctx_q32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }
    ctx->out = 0;
}
