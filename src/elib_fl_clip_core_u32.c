/* elib_fl_clip_core_u32.c - Amplitude Limiter (Uint32) */

#include "elib_fl_clip_core.h"

elib_fl_err_t elib_fl_clip_init_u32(elib_fl_clip_ctx_u32_t *ctx, uint32_t threshold)
{
    if (ctx == NULL || threshold == 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->threshold = threshold;
    ctx->out = 0;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_clip_set_u32(elib_fl_clip_ctx_u32_t *ctx, uint32_t value)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return ELIB_FL_ERR_NOT_INITIALIZED;
    }

    ctx->out = value;

    return ELIB_FL_OK;
}

uint32_t elib_fl_clip_update_u32(elib_fl_clip_ctx_u32_t *ctx, uint32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    int64_t diff = (int64_t)in - (int64_t)ctx->out;
    if (diff < 0) {
        diff = -diff;
    }

    if (diff <= (int64_t)ctx->threshold) {
        ctx->out = in;
    }

    return ctx->out;
}

void elib_fl_clip_reset_u32(elib_fl_clip_ctx_u32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->out = 0;
}
