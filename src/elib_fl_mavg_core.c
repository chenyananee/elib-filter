/* elib_fl_mavg_core.c - Moving Average Filter (Float) */

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
        ctx->buf[i] = 0.0f;
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
    }

    ctx->buf[ctx->idx] = in;
    ctx->sum += in;
    ctx->idx = (ctx->idx + 1) % ctx->size;
    ctx->count++;

    uint32_t n = ctx->count;
    if (n > ctx->size) {
        n = ctx->size;
    }

    return ctx->sum / (float)n;
}

float elib_fl_mavg_oneshot_f(const float *buf, uint32_t size)
{
    if (buf == NULL || size == 0) {
        return 0.0f;
    }

    float sum = 0.0f;
    for (uint32_t i = 0; i < size; i++) {
        sum += buf[i];
    }

    return sum / (float)size;
}

uint32_t elib_fl_mavg_warmup_f(const elib_fl_mavg_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    if (ctx->count >= ctx->size) {
        return 0;
    }

    return ctx->size - ctx->count;
}

void elib_fl_mavg_reset_f(elib_fl_mavg_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->sum = 0.0f;
    ctx->idx = 0;
    ctx->count = 0;

    for (uint32_t i = 0; i < ctx->size; i++) {
        ctx->buf[i] = 0.0f;
    }
}
