/* elib_fl_clip_core_q32.c - Amplitude Limiter (Q32) */

#include "elib_fl_clip_core.h"

elib_fl_err_t elib_fl_clip_init_q32(elib_fl_clip_ctx_q32_t *ctx, int32_t threshold, uint8_t n)
{
    if (ctx == NULL || threshold <= 0 || n == 0 || n > 31) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->threshold = threshold;
    ctx->n = n;
    ctx->out = 0;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_clip_set_q32(elib_fl_clip_ctx_q32_t *ctx, int32_t value)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return ELIB_FL_ERR_NOT_INITIALIZED;
    }

    ctx->out = value;

    return ELIB_FL_OK;
}

int32_t elib_fl_clip_update_q32(elib_fl_clip_ctx_q32_t *ctx, int32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    int32_t diff = in - ctx->out;
    if (diff < 0) {
        diff = -diff;
    }

    if (diff <= ctx->threshold) {
        ctx->out = in;
    }

    return ctx->out;
}

int32_t elib_fl_clip_oneshot_q32(int32_t threshold, int32_t in, int32_t prev)
{
    int32_t diff = in - prev;
    if (diff < 0) {
        diff = -diff;
    }

    if (diff <= threshold) {
        return in;
    }
    return prev;
}

void elib_fl_clip_reset_q32(elib_fl_clip_ctx_q32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }
    ctx->out = 0;
}
