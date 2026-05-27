/* elib_fl_avg_core_q32.c - Average Filter (Q32) */

#include "elib_fl_avg_core.h"

elib_fl_err_t elib_fl_avg_init_q32(elib_fl_avg_ctx_q32_t *ctx, uint32_t size)
{
    if (ctx == NULL || size == 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->size = size;
    ctx->sum = 0;
    ctx->count = 0;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

int32_t elib_fl_avg_update_q32(elib_fl_avg_ctx_q32_t *ctx, int32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    ctx->sum += in;
    ctx->count++;

    if (ctx->count < ctx->size) {
        return 0;
    }

    int32_t result = ctx->sum / (int32_t)ctx->size;
    ctx->sum = 0;
    ctx->count = 0;

    return result;
}

int32_t elib_fl_avg_oneshot_q32(const int32_t *buf, uint32_t size)
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

void elib_fl_avg_reset_q32(elib_fl_avg_ctx_q32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }
    ctx->sum = 0;
    ctx->count = 0;
}
