/* elib_fl_hyst_core_f.c - Boundary Hysteresis Limiter (Float)
 *
 * One decision edge with a sticky band and a snap-through on jumps.
 * With s = in - boundary (boundary b, hysteresis width h, break
 * width S, S > h enforced at init):
 *
 *   LOW:  s <= 0 -> actual in
 *         0 < s <= h -> sticky clamp to b, unless |in - out| > S:
 *                       snap through, HIGH
 *         s > h  -> actual in, HIGH
 *   HIGH: s > 0  -> actual in
 *         -h < s <= 0 -> hold, unless |in - out| > S:
 *                       snap through, side follows in
 *         s <= -h -> actual in, LOW
 *
 * NaN input: swallowed (s != s probe), output held -- family
 * behaviour. NaN configuration is rejected at init (it would poison
 * every update). Pure comparisons, no accumulation: no overflow
 * surface. */

#include "elib_fl.h"
#include <string.h>

elib_fl_err_t elib_fl_hyst_init_f(elib_fl_hyst_ctx_f_t *ctx, float boundary, float hyst, float snap)
{
    if (ctx == NULL || boundary != boundary || hyst != hyst || snap != snap ||
        hyst <= 0.0f || snap <= hyst) {
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

elib_fl_err_t elib_fl_hyst_set_f(elib_fl_hyst_ctx_f_t *ctx, float value)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return ELIB_FL_ERR_NOT_INITIALIZED;
    }

    ctx->out = value;
    ctx->bit_flags.above = (value > ctx->boundary) ? 1u : 0u;

    return ELIB_FL_OK;
}

float elib_fl_hyst_update_f(elib_fl_hyst_ctx_f_t *ctx, float in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0.0f;
    }

    float s = in - ctx->boundary;
    if (s != s) {
        /* NaN input: no comparable distance, hold the output */
        return ctx->out;
    }

    if (ctx->bit_flags.above) {
        /* HIGH: output sits strictly above the boundary */
        if (s > 0.0f) {
            ctx->out = in;
        } else if (s <= -(ctx->hyst)) {
            /* fell a full hysteresis width below: honest, LOW */
            ctx->out = in;
            ctx->bit_flags.above = 0u;
        } else {
            /* (-h, 0]: the hold band -- unless the step is a jump */
            float d = in - ctx->out;
            if (d < 0.0f) {
                d = -d;
            }
            if (d > ctx->snap) {
                ctx->out = in;
                ctx->bit_flags.above = (in > ctx->boundary) ? 1u : 0u;
            }
            /* else: hold last output unchanged */
        }
    } else {
        /* LOW: output at/below the boundary */
        if (s <= 0.0f) {
            ctx->out = in;
        } else if (s <= ctx->hyst) {
            /* (0, h]: sticky clamp to the boundary -- unless the
             * step against the last output is a genuine jump */
            float d = in - ctx->out;
            if (d < 0.0f) {
                d = -d;
            }
            if (d > ctx->snap) {
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

void elib_fl_hyst_reset_f(elib_fl_hyst_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->out = 0.0f;
    ctx->bit_flags.above = 0u;
}
