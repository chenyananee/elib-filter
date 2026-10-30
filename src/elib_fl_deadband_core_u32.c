/* elib_fl_deadband_core_u32.c - Dead-Zone Filter (Uint32)
 *
 * Small changes are IGNORED (output held), large ones are followed.
 * The signed |in - out| difference is computed in int64, so the
 * full uint32 input range is safe. */

#include "elib_fl.h"
#include <string.h>

elib_fl_err_t elib_fl_deadband_init_u32(elib_fl_deadband_ctx_u32_t *ctx, uint32_t threshold)
{
    if (ctx == NULL || threshold == 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->threshold = threshold;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_deadband_set_u32(elib_fl_deadband_ctx_u32_t *ctx, uint32_t value)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return ELIB_FL_ERR_NOT_INITIALIZED;
    }

    ctx->out = value;

    return ELIB_FL_OK;
}

uint32_t elib_fl_deadband_update_u32(elib_fl_deadband_ctx_u32_t *ctx, uint32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    int64_t diff = (int64_t)in - (int64_t)ctx->out;
    if (diff < 0) {
        diff = -diff;
    }

    if (diff > (int64_t)ctx->threshold) {
        ctx->out = in;
    }

    return ctx->out;
}

void elib_fl_deadband_reset_u32(elib_fl_deadband_ctx_u32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->out = 0;
}
