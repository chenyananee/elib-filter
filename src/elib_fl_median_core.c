/* elib_fl_median_core.c - Median Filter (Float) */

#include "elib_fl_median_core.h"

static void median_insertion_sort_f(float *buf, uint32_t size)
{
    for (uint32_t i = 1; i < size; i++) {
        float key = buf[i];
        int32_t j = (int32_t)i - 1;
        while (j >= 0 && buf[j] > key) {
            buf[j + 1] = buf[j];
            j--;
        }
        buf[j + 1] = key;
    }
}

static float median_compute_f(float *buf, uint32_t size)
{
    median_insertion_sort_f(buf, size);
    return buf[size / 2];
}

elib_fl_err_t elib_fl_median_init_f(elib_fl_median_ctx_f_t *ctx, float *buf, uint32_t size)
{
    if (ctx == NULL || buf == NULL || size == 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    ctx->buf = buf;
    ctx->size = size;
    ctx->count = 0;
    ctx->bit_flags.initialized = 1;

    return ELIB_FL_OK;
}

float elib_fl_median_update_f(elib_fl_median_ctx_f_t *ctx, float in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0.0f;
    }

    ctx->buf[ctx->count] = in;
    ctx->count++;

    if (ctx->count < ctx->size) {
        return 0.0f;
    }

    float result = median_compute_f(ctx->buf, ctx->size);
    ctx->count = 0;

    return result;
}

float elib_fl_median_oneshot_f(float *buf, uint32_t size)
{
    if (buf == NULL || size == 0) {
        return 0.0f;
    }

    return median_compute_f(buf, size);
}

void elib_fl_median_reset_f(elib_fl_median_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }
    ctx->count = 0;
}
