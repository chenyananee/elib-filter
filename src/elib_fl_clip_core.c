/* elib_fl_clip_core.c - Amplitude Limiter (Float) */

#include "elib_fl_clip_core.h"

elib_fl_err_t elib_fl_clip_init_f(elib_fl_clip_ctx_f_t *ctx, float threshold)
{
    if (ctx == NULL || threshold <= 0.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->threshold = threshold;
    ctx->out = 0.0f;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_clip_set_f(elib_fl_clip_ctx_f_t *ctx, float value)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return ELIB_FL_ERR_NOT_INITIALIZED;
    }

    ctx->out = value;

    return ELIB_FL_OK;
}

float elib_fl_clip_update_f(elib_fl_clip_ctx_f_t *ctx, float in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0.0f;
    }

    float diff = in - ctx->out;
    if (diff < 0.0f) {
        diff = -diff;
    }

    if (diff <= ctx->threshold) {
        ctx->out = in;
    }

    return ctx->out;
}

float elib_fl_clip_oneshot_f(float threshold, float in, float prev)
{
    float diff = in - prev;
    if (diff < 0.0f) {
        diff = -diff;
    }

    if (diff <= threshold) {
        return in;
    }
    return prev;
}

void elib_fl_clip_reset_f(elib_fl_clip_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }
    ctx->out = 0.0f;
}
