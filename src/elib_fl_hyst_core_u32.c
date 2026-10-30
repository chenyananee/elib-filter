/* elib_fl_hyst_core_u32.c - Boundary Hysteresis Limiter (Uint32)
 *
 * One decision edge with a sticky band and a snap-through on jumps
 * (see elib_fl_hyst_core_f.c for the full rule table). The signed
 * difference of two uint32 values is computed in int64, so the full
 * uint32 input range is safe (no accumulation, no overflow surface). */

#include "elib_fl.h"
#include <string.h>

elib_fl_err_t elib_fl_hyst_init_u32(elib_fl_hyst_ctx_u32_t *ctx, uint32_t boundary, uint32_t hyst, uint32_t snap)
{
    if (ctx == NULL || hyst == 0 || snap <= hyst) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->boundary = boundary;
    ctx->hyst = hyst;
    ctx->snap = snap;
    ctx->bit_flags.initialized = 1;
    /* out = 0, above = 0 (LOW) */

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_hyst_set_u32(elib_fl_hyst_ctx_u32_t *ctx, uint32_t value)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return ELIB_FL_ERR_NOT_INITIALIZED;
    }

    ctx->out = value;
    ctx->bit_flags.above = (value > ctx->boundary) ? 1u : 0u;

    return ELIB_FL_OK;
}

uint32_t elib_fl_hyst_update_u32(elib_fl_hyst_ctx_u32_t *ctx, uint32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    int64_t s = (int64_t)in - (int64_t)ctx->boundary;

    if (ctx->bit_flags.above) {
        /* HIGH: output sits strictly above the boundary */
        if (s > 0) {
            ctx->out = in;
        } else if (s <= -((int64_t)ctx->hyst)) {
            /* fell a full hysteresis width below: honest, LOW */
            ctx->out = in;
            ctx->bit_flags.above = 0u;
        } else {
            /* (-h, 0]: the hold band -- unless the step is a jump */
            int64_t d = (int64_t)in - (int64_t)ctx->out;
            if (d < 0) {
                d = -d;
            }
            if (d > (int64_t)ctx->snap) {
                ctx->out = in;
                ctx->bit_flags.above = (in > ctx->boundary) ? 1u : 0u;
            }
            /* else: hold last output unchanged */
        }
    } else {
        /* LOW: output at/below the boundary */
        if (s <= 0) {
            ctx->out = in;
        } else if (s <= (int64_t)ctx->hyst) {
            /* (0, h]: sticky clamp to the boundary -- unless the
             * step against the last output is a genuine jump */
            int64_t d = (int64_t)in - (int64_t)ctx->out;
            if (d < 0) {
                d = -d;
            }
            if (d > (int64_t)ctx->snap) {
                /* 0 -> 6 style jump straight through the band */
                ctx->out = in;
                ctx->bit_flags.above = 1u;
            } else {
                ctx->out = ctx->boundary;
            }
        } else {
            /* s > h: crossed the envelope, honest, HIGH */
            ctx->out = in;
            ctx->bit_flags.above = 1u;
        }
    }

    return ctx->out;
}

void elib_fl_hyst_reset_u32(elib_fl_hyst_ctx_u32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->out = 0;
    ctx->bit_flags.above = 0u;
}
