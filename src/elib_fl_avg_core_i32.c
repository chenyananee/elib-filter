/* elib_fl_avg_core_i32.c - Batch Average Filter (Int32)
 *
 * Non-overlapping window with a 64-BIT accumulator: any int32 values
 * over any window length accumulate without overflow (the v1 int32
 * accumulator overflowed with undefined behavior). The mean truncates
 * toward zero. */

#include "elib_fl.h"
#include <string.h>

elib_fl_err_t elib_fl_avg_init_i32(elib_fl_avg_ctx_i32_t *ctx, uint32_t size)
{
    if (ctx == NULL || size == 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->size = size;
    ctx->bit_flags.initialized = 1;

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

    int32_t result = (int32_t)(ctx->sum / (int64_t)ctx->size);
    ctx->sum = 0;
    ctx->count = 0;
    ctx->bit_flags.ready = 1;

    return result;
}

uint32_t elib_fl_avg_ready_i32(const elib_fl_avg_ctx_i32_t *ctx)
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
