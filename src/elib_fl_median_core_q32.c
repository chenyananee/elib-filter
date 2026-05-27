/* elib_fl_median_core_q32.c - Median Filter (Q32) */

#include "elib_fl_median_core.h"

static void median_insertion_sort_q32(int32_t *buf, uint32_t size)
{
    for (uint32_t i = 1; i < size; i++) {
        int32_t key = buf[i];
        int32_t j = (int32_t)i - 1;
        while (j >= 0 && buf[j] > key) {
            buf[j + 1] = buf[j];
            j--;
        }
        buf[j + 1] = key;
    }
}

static int32_t median_compute_q32(int32_t *buf, uint32_t size)
{
    median_insertion_sort_q32(buf, size);
    return buf[size / 2];
}

elib_fl_err_t elib_fl_median_init_q32(elib_fl_median_ctx_q32_t *ctx, int32_t *buf, uint32_t size)
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

int32_t elib_fl_median_update_q32(elib_fl_median_ctx_q32_t *ctx, int32_t in)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return 0;
    }

    ctx->buf[ctx->count] = in;
    ctx->count++;

    if (ctx->count < ctx->size) {
        return 0;
    }

    int32_t result = median_compute_q32(ctx->buf, ctx->size);
    ctx->count = 0;

    return result;
}

int32_t elib_fl_median_oneshot_q32(int32_t *buf, uint32_t size)
{
    if (buf == NULL || size == 0) {
        return 0;
    }

    return median_compute_q32(buf, size);
}

void elib_fl_median_reset_q32(elib_fl_median_ctx_q32_t *ctx)
{
    if (ctx == NULL || !ctx->bit_flags.initialized) {
        return;
    }
    ctx->count = 0;
}
