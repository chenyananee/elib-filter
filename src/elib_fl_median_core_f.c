/* elib_fl_median_core_f.c - Batch Median Filter (Float)
 *
 * Non-overlapping window: one median every `size` updates, computed
 * on a scratch insertion sort of the user-owned buffer (the buffer
 * is sorted IN PLACE -- its contents are destroyed at every batch
 * boundary; they would be overwritten by the next batch anyway). */

#include "elib_fl.h"
#include <string.h>

static void median_sort_f(float *buf, uint32_t size)
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

elib_fl_err_t elib_fl_median_init_f(elib_fl_median_ctx_f_t *ctx, float *buf, uint32_t size)
{
    if (ctx == NULL || buf == NULL || size == 0) {
        return ELIB_FL_ERR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->buf = buf;
    ctx->size = size;
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
        ctx->bit_flags.ready = 0;
        return 0.0f;
    }

    median_sort_f(ctx->buf, ctx->size);
    float result = ctx->buf[ctx->size / 2];
    ctx->count = 0;
    ctx->bit_flags.ready = 1;

    return result;
}

uint32_t elib_fl_median_ready_f(const elib_fl_median_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    return ctx->bit_flags.ready ? 1u : 0u;
}

void elib_fl_median_reset_f(elib_fl_median_ctx_f_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }

    ctx->count = 0;
    ctx->bit_flags.ready = 0;
}
