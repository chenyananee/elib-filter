/* elib_fl_avg_core_f.c - Average Filter (Float) */

#include "elib_fl_avg_core.h"

elib_fl_err_t elib_fl_avg_init_f(elib_fl_avg_ctx_f_t *ctx, uint32_t size)
{
    if (ctx == NULL || size == 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->size = size;
    ctx->sum = 0.0f;
    ctx->count = 0;
    ctx->bit_flags.initialized = 1;
    ctx->bit_flags.ready = 0;

    return ELIB_FL_OK;
}

float elib_fl_avg_update_f(elib_fl_avg_ctx_f_t *ctx, float in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0.0f;
    }

    ctx->sum += in;
    ctx->count++;

    if (ctx->count < ctx->size) {
        ctx->bit_flags.ready = 0;
        return 0.0f;
    }

    float result = ctx->sum / (float)ctx->size;
    ctx->sum = 0.0f;
    ctx->count = 0;
    ctx->bit_flags.ready = 1;

    return result;
}

uint32_t elib_fl_avg_warmup_f(const elib_fl_avg_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    return ctx->bit_flags.ready ? 1u : 0u;
}

void elib_fl_avg_reset_f(elib_fl_avg_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->sum = 0.0f;
    ctx->count = 0;
    ctx->bit_flags.ready = 0;
}
