/* elib_fl_lpf1_core_i16.c - First-Order Low-Pass Filter (Int16, Qn alpha)
 *
 * Intermediates are int32 and bound-provably safe: |diff| <= 2^16-1
 * (i16 range 32767+32768) and alpha <= 2^15-1, so
 * alpha * diff <= (2^15-1) * (2^16-1) < 2^31 -- no 64-bit step. */

#include "elib_fl.h"
#include <string.h>

#define ELIB_FL_LPF1_PI 3.14159265358979323846f

/* Quantize a float alpha to Qn; keep the 0 < alpha < 2^n invariant
 * absolute (a rounded-up 2^n would turn the filter into a
 * pass-through). Currently unreachable via init_tau/init_fc (both
 * cap alpha at 0.5), but it is the single guard that matters. */
static elib_fl_err_t lpf1_i16_set_alpha(elib_fl_lpf1_ctx_i16_t *ctx, float alpha_f, uint8_t n)
{
    int32_t scaled = (int32_t)((double)alpha_f * (double)(1 << n) + 0.5);
    if (scaled <= 0 || scaled >= (1 << n) || scaled > INT16_MAX) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->alpha = (int16_t)scaled;
    ctx->n = n;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_lpf1_init_i16(elib_fl_lpf1_ctx_i16_t *ctx, int16_t alpha, uint8_t n)
{
    if (ctx == NULL || n == 0 || n > 15 || alpha <= 0 || (int32_t)alpha >= (1 << n)) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->alpha = alpha;
    ctx->n = n;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_lpf1_init_tau_i16(elib_fl_lpf1_ctx_i16_t *ctx, float tau, float dt, uint8_t n)
{
    if (ctx == NULL || tau <= 0.0f || dt <= 0.0f || n == 0 || n > 15) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    float alpha_f = dt / (tau + dt);
    if (alpha_f <= 0.0f || alpha_f >= 1.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    return lpf1_i16_set_alpha(ctx, alpha_f, n);
}

elib_fl_err_t elib_fl_lpf1_init_fc_i16(elib_fl_lpf1_ctx_i16_t *ctx, float fc, float fs, uint8_t n)
{
    if (ctx == NULL || fc <= 0.0f || fs <= 0.0f || n == 0 || n > 15) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    float rc = 1.0f / (2.0f * ELIB_FL_LPF1_PI * fc);
    float alpha_f = (1.0f / fs) / (rc + (1.0f / fs));
    if (alpha_f <= 0.0f || alpha_f >= 1.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    return lpf1_i16_set_alpha(ctx, alpha_f, n);
}

int16_t elib_fl_lpf1_update_i16(elib_fl_lpf1_ctx_i16_t *ctx, int16_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    /* |diff| <= 65535, alpha <= 32767: 32767 * 65535 = 2147418705
     * < INT32_MAX (margin ~65k), the product cannot overflow */
    int32_t diff = (int32_t)in - (int32_t)ctx->out;
    ctx->out = (int16_t)((int32_t)ctx->out + ((diff * (int32_t)ctx->alpha) >> ctx->n));

    return ctx->out;
}

void elib_fl_lpf1_reset_i16(elib_fl_lpf1_ctx_i16_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->out = 0;
}
