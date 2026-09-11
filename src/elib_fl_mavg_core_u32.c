/* elib_fl_mavg_core_u32.c - Moving Average Filter (Uint32) */

#include "elib_fl_mavg_core.h"

elib_fl_err_t elib_fl_mavg_init_u32(elib_fl_mavg_ctx_u32_t *ctx, uint32_t *buf, uint32_t size)
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
        buf[i] = 0;
    }

    return ELIB_FL_OK;
}

uint32_t elib_fl_mavg_update_u32(elib_fl_mavg_ctx_u32_t *ctx, uint32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    if (ctx->count >= ctx->size) {
        ctx->sum -= ctx->buf[ctx->idx];
    } else {
        ctx->count++;
    }

    ctx->buf[ctx->idx] = in;
    ctx->sum += in;
    ctx->idx = (ctx->idx + 1) % ctx->size;

    return ctx->sum / ctx->count;
}

uint32_t elib_fl_mavg_warmup_u32(const elib_fl_mavg_ctx_u32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    return (ctx->count >= ctx->size) ? 1u : 0u;
}

void elib_fl_mavg_reset_u32(elib_fl_mavg_ctx_u32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->sum = 0;
    ctx->idx = 0;
    ctx->count = 0;
}
