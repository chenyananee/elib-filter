/* elib_fl_deadband_core_i16.c - Dead-Zone Filter (Int16)
 *
 * Small changes are IGNORED (output held), large ones are followed.
 * The |in - out| difference fits int32 with room to spare
 * (|diff| <= 2^16-1). */

#include "elib_fl.h"
#include <string.h>

elib_fl_err_t elib_fl_deadband_init_i16(elib_fl_deadband_ctx_i16_t *ctx, int16_t threshold)
{
    if (ctx == NULL || threshold <= 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->threshold = threshold;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_deadband_set_i16(elib_fl_deadband_ctx_i16_t *ctx, int16_t value)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return ELIB_FL_ERR_NOT_INITIALIZED;
    }

    ctx->out = value;

    return ELIB_FL_OK;
}

int16_t elib_fl_deadband_update_i16(elib_fl_deadband_ctx_i16_t *ctx, int16_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    int32_t diff = (int32_t)in - (int32_t)ctx->out;
    if (diff < 0) {
        diff = -diff;
    }

    if (diff > (int32_t)ctx->threshold) {
        ctx->out = in;
    }

    return ctx->out;
}

void elib_fl_deadband_reset_i16(elib_fl_deadband_ctx_i16_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->out = 0;
}
