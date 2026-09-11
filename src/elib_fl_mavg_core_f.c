/* elib_fl_mavg_core_f.c - Moving Average Filter (Float) */

#include "elib_fl_mavg_core.h"

elib_fl_err_t elib_fl_mavg_init_f(elib_fl_mavg_ctx_f_t *ctx, float *buf, uint32_t size)
{
    if (ctx == NULL || buf == NULL || size == 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->buf = buf;
    ctx->size = size;
    ctx->sum = 0.0f;
    ctx->idx = 0;
    ctx->count = 0;
    ctx->bit_flags.initialized = 1;

    for (uint32_t i = 0; i < size; i++) {
        buf[i] = 0.0f;
    }

    return ELIB_FL_OK;
}

float elib_fl_mavg_update_f(elib_fl_mavg_ctx_f_t *ctx, float in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0.0f;
    }

    if (ctx->count >= ctx->size) {
        ctx->sum -= ctx->buf[ctx->idx];
    } else {
        ctx->count++;
    }

    ctx->buf[ctx->idx] = in;
    ctx->sum += in;
    ctx->idx = (ctx->idx + 1) % ctx->size;

    return ctx->sum / (float)ctx->count;
}

uint32_t elib_fl_mavg_warmup_f(const elib_fl_mavg_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    return (ctx->count >= ctx->size) ? 1u : 0u;
}

void elib_fl_mavg_reset_f(elib_fl_mavg_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->sum = 0.0f;
    ctx->idx = 0;
    ctx->count = 0;
}
