/* elib_fl_hyst_core_u16.c - Boundary Hysteresis Limiter (Uint16)
 *
 * One decision edge with a sticky band and a snap-through on jumps
 * (see elib_fl_hyst_core_f.c for the full rule table). The signed
 * difference of two uint16 values fits int32 with room to spare
 * (|s|, |d| <= 2^16-1), so the int32 difference pipeline is bound-
 * provably safe. */

#include "elib_fl.h"
#include <string.h>

elib_fl_err_t elib_fl_hyst_init_u16(elib_fl_hyst_ctx_u16_t *ctx, uint16_t boundary, uint16_t hyst, uint16_t snap)
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

elib_fl_err_t elib_fl_hyst_set_u16(elib_fl_hyst_ctx_u16_t *ctx, uint16_t value)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return ELIB_FL_ERR_NOT_INITIALIZED;
    }

    ctx->out = value;
    ctx->bit_flags.above = (value > ctx->boundary) ? 1u : 0u;

    return ELIB_FL_OK;
}

uint16_t elib_fl_hyst_update_u16(elib_fl_hyst_ctx_u16_t *ctx, uint16_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    int32_t s = (int32_t)in - (int32_t)ctx->boundary;

    if (ctx->bit_flags.above) {
        /* HIGH: output sits strictly above the boundary */
        if (s > 0) {
            ctx->out = in;
        } else if (s <= -(int32_t)ctx->hyst) {
            /* fell a full hysteresis width below: honest, LOW */
            ctx->out = in;
            ctx->bit_flags.above = 0u;
        } else {
            /* (-h, 0]: the hold band -- unless the step is a jump */
            int32_t d = (int32_t)in - (int32_t)ctx->out;
            if (d < 0) {
                d = -d;
            }
            if (d > (int32_t)ctx->snap) {
                ctx->out = in;
                ctx->bit_flags.above = (in > ctx->boundary) ? 1u : 0u;
            }
            /* else: hold last output unchanged */
        }
    } else {
        /* LOW: output at/below the boundary */
        if (s <= 0) {
            ctx->out = in;
        } else if (s <= (int32_t)ctx->hyst) {
            /* (0, h]: sticky clamp to the boundary -- unless the
             * step against the last output is a genuine jump */
            int32_t d = (int32_t)in - (int32_t)ctx->out;
            if (d < 0) {
                d = -d;
            }
            if (d > (int32_t)ctx->snap) {
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

void elib_fl_hyst_reset_u16(elib_fl_hyst_ctx_u16_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->out = 0;
    ctx->bit_flags.above = 0u;
}
