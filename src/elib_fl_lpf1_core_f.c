/* elib_fl_lpf1_core_f.c - First-Order Low-Pass Filter (Float) */

#include "elib_fl.h"
#include <string.h>

#define ELIB_FL_LPF1_PI 3.14159265358979323846f

elib_fl_err_t elib_fl_lpf1_init_f(elib_fl_lpf1_ctx_f_t *ctx, float alpha)
{
    if (ctx == NULL || alpha <= 0.0f || alpha >= 1.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->alpha = alpha;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_lpf1_init_tau_f(elib_fl_lpf1_ctx_f_t *ctx, float tau, float dt)
{
    if (ctx == NULL || tau <= 0.0f || dt <= 0.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    float alpha = dt / (tau + dt);
    if (alpha <= 0.0f || alpha >= 1.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->alpha = alpha;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

elib_fl_err_t elib_fl_lpf1_init_fc_f(elib_fl_lpf1_ctx_f_t *ctx, float fc, float fs)
{
    if (ctx == NULL || fc <= 0.0f || fs <= 0.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    float rc = 1.0f / (2.0f * ELIB_FL_LPF1_PI * fc);
    float alpha = (1.0f / fs) / (rc + (1.0f / fs));
    if (alpha <= 0.0f || alpha >= 1.0f) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->alpha = alpha;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

float elib_fl_lpf1_update_f(elib_fl_lpf1_ctx_f_t *ctx, float in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0.0f;
    }

    ctx->out = ctx->out + ctx->alpha * (in - ctx->out);

    return ctx->out;
}

void elib_fl_lpf1_reset_f(elib_fl_lpf1_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->out = 0.0f;
}
