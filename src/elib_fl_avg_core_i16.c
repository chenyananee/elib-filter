/* elib_fl_avg_core_i16.c - Batch Average Filter (Int16)
 *
 * Non-overlapping window with a 32-bit accumulator. It is bound-
 * provably overflow-free because init REJECTS windows where the
 * worst-case sum would not fit: with size <= 65535,
 * |sum| <= 65535 * 32768 = 2147420416 < 2^31. Larger windows are
 * rejected at init (illegal config never reaches runtime). */

#include "elib_fl.h"
#include <string.h>

#define ELIB_FL_AVG_I16_MAX_SIZE 65535u

elib_fl_err_t elib_fl_avg_init_i16(elib_fl_avg_ctx_i16_t *ctx, uint32_t size)
{
    if (ctx == NULL || size == 0 || size > ELIB_FL_AVG_I16_MAX_SIZE) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->size = size;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

int16_t elib_fl_avg_update_i16(elib_fl_avg_ctx_i16_t *ctx, int16_t in)
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

    int16_t result = (int16_t)(ctx->sum / (int32_t)ctx->size);
    ctx->sum = 0;
    ctx->count = 0;
    ctx->bit_flags.ready = 1;

    return result;
}

uint32_t elib_fl_avg_ready_i16(const elib_fl_avg_ctx_i16_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    return ctx->bit_flags.ready ? 1u : 0u;
}

void elib_fl_avg_reset_i16(elib_fl_avg_ctx_i16_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->sum = 0;
    ctx->count = 0;
    ctx->bit_flags.ready = 0;
}
