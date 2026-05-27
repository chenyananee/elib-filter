/* elib_fl_mavg_core_q32.c - Moving Average Filter (Q32) */

#include "elib_fl_mavg_core.h"

elib_fl_err_t elib_fl_mavg_init_q32(elib_fl_mavg_ctx_q32_t *ctx, int32_t *buf, uint32_t size)
{
    if (ctx == NULL || buf == NULL || size == 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->buf = buf;
    ctx->size = size;
    ctx->sum = 0;
    ctx->idx = 0;
    ctx->count = 0;
    ctx->bit_flags.initialized = 1;

    for (uint32_t i = 0; i < size; i++) {
        ctx->buf[i] = 0;
    }

    return ELIB_FL_OK;
}

int32_t elib_fl_mavg_update_q32(elib_fl_mavg_ctx_q32_t *ctx, int32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
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

    return ctx->sum / (int32_t)n;
}

int32_t elib_fl_mavg_oneshot_q32(const int32_t *buf, uint32_t size)
{
    if (buf == NULL || size == 0) {
        return 0;
    }

    int32_t sum = 0;
    for (uint32_t i = 0; i < size; i++) {
        sum += buf[i];
    }

    return sum / (int32_t)size;
}

uint32_t elib_fl_mavg_warmup_q32(const elib_fl_mavg_ctx_q32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    if (ctx->count >= ctx->size) {
        return 0;
    }

    return ctx->size - ctx->count;
}

void elib_fl_mavg_reset_q32(elib_fl_mavg_ctx_q32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->sum = 0;
    ctx->idx = 0;
    ctx->count = 0;

    for (uint32_t i = 0; i < ctx->size; i++) {
        ctx->buf[i] = 0;
    }
}
