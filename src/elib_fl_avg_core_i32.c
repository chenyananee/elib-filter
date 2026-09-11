/* elib_fl_avg_core_i32.c - Average Filter (Int32) */

#include "elib_fl_avg_core.h"

elib_fl_err_t elib_fl_avg_init_i32(elib_fl_avg_ctx_i32_t *ctx, uint32_t size)
{
    if (ctx == NULL || size == 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->size = size;
    ctx->sum = 0;
    ctx->count = 0;
    ctx->bit_flags.initialized = 1;
    ctx->bit_flags.ready = 0;

    return ELIB_FL_OK;
}

int32_t elib_fl_avg_update_i32(elib_fl_avg_ctx_i32_t *ctx, int32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    ctx->sum += in;
    ctx->count++;

    if (ctx->count < ctx->size) {
        ctx->bit_flags.ready = 0;
        return 0;
    }

    int32_t result = ctx->sum / (int32_t)ctx->size;
    ctx->sum = 0;
    ctx->count = 0;
    ctx->bit_flags.ready = 1;

    return result;
}

uint32_t elib_fl_avg_warmup_i32(const elib_fl_avg_ctx_i32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    return ctx->bit_flags.ready ? 1u : 0u;
}

void elib_fl_avg_reset_i32(elib_fl_avg_ctx_i32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->sum = 0;
    ctx->count = 0;
    ctx->bit_flags.ready = 0;
}
