/* elib_fl_deadband_core_f.c - Dead-Zone Filter (Float)
 *
 * Small changes are IGNORED (output held), large ones are followed:
 * the mirror image of slew. threshold 0 would follow every nonzero
 * change (pure pass-through) and is rejected at init. */

#include "elib_fl.h"
#include <string.h>

elib_fl_err_t elib_fl_deadband_init_f(elib_fl_deadband_ctx_f_t *ctx, float threshold)
{
    if (ctx == NULL || threshold <= 0.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->threshold = threshold;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_deadband_set_f(elib_fl_deadband_ctx_f_t *ctx, float value)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return ELIB_FL_ERR_NOT_INITIALIZED;
    }

    ctx->out = value;

    return ELIB_FL_OK;
}

float elib_fl_deadband_update_f(elib_fl_deadband_ctx_f_t *ctx, float in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0.0f;
    }

    float diff = in - ctx->out;
    if (diff < 0.0f) {
        diff = -diff;
    }

    if (diff > ctx->threshold) {
        ctx->out = in;
    }

    return ctx->out;
}

void elib_fl_deadband_reset_f(elib_fl_deadband_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->out = 0.0f;
}
