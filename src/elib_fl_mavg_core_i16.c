/* elib_fl_mavg_core_i16.c - Sliding-Window Mean Filter (Int16)
 *
 * O(1) sliding average: ring buffer + running sum, with a 32-bit
 * accumulator. It is bound-provably overflow-free because init
 * REJECTS windows where the worst-case sum would not fit: with
 * size <= 65535, |sum| <= 65535 * 32768 < 2^31. Larger windows are
 * rejected at init (illegal config never reaches runtime). */

#include "elib_fl.h"
#include <string.h>

#define ELIB_FL_MAVG_I16_MAX_SIZE 65535u

elib_fl_err_t elib_fl_mavg_init_i16(elib_fl_mavg_ctx_i16_t *ctx, int16_t *buf, uint32_t size)
{
    if (ctx == NULL || buf == NULL || size == 0 || size > ELIB_FL_MAVG_I16_MAX_SIZE) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->buf = buf;
    ctx->size = size;
    ctx->bit_flags.initialized = 1;

    for (uint32_t i = 0; i < size; i++) {
        buf[i] = 0;
    }

    return ELIB_FL_OK;
}

int16_t elib_fl_mavg_update_i16(elib_fl_mavg_ctx_i16_t *ctx, int16_t in)
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

    return (int16_t)(ctx->sum / (int32_t)ctx->count);
}

uint32_t elib_fl_mavg_ready_i16(const elib_fl_mavg_ctx_i16_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    return (ctx->count >= ctx->size) ? 1u : 0u;
}

void elib_fl_mavg_reset_i16(elib_fl_mavg_ctx_i16_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->sum = 0;
    ctx->idx = 0;
    ctx->count = 0;
}
