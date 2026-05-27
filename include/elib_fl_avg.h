/* elib_fl_avg.h - Average Filter */

#ifndef ELIB_FL_AVG_H
#define ELIB_FL_AVG_H

#include "elib_fl_err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Float context                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    float    sum;
    uint32_t size;
    uint32_t count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_avg_ctx_f_t;

elib_fl_err_t elib_fl_avg_init_f(elib_fl_avg_ctx_f_t *ctx, uint32_t size);
float         elib_fl_avg_update_f(elib_fl_avg_ctx_f_t *ctx, float in);
float         elib_fl_avg_oneshot_f(const float *buf, uint32_t size);
void          elib_fl_avg_reset_f(elib_fl_avg_ctx_f_t *ctx);

/* ------------------------------------------------------------------ */
/*  Q32 context                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    int32_t  sum;          /* accumulated sum, max = size * INT32_MAX */
    uint32_t size;
    uint32_t count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_avg_ctx_q32_t;

elib_fl_err_t elib_fl_avg_init_q32(elib_fl_avg_ctx_q32_t *ctx, uint32_t size);
int32_t       elib_fl_avg_update_q32(elib_fl_avg_ctx_q32_t *ctx, int32_t in);
int32_t       elib_fl_avg_oneshot_q32(const int32_t *buf, uint32_t size);
void          elib_fl_avg_reset_q32(elib_fl_avg_ctx_q32_t *ctx);

/* ------------------------------------------------------------------ */
/*  Generic API                                                        */
/* ------------------------------------------------------------------ */

#define elib_fl_avg_init(ctx, ...) \
    _Generic((ctx), \
        elib_fl_avg_ctx_f_t *:   elib_fl_avg_init_f((ctx), __VA_ARGS__), \
        elib_fl_avg_ctx_q32_t *: elib_fl_avg_init_q32((ctx), __VA_ARGS__))

#define elib_fl_avg_update(ctx, in) \
    _Generic((ctx), \
        elib_fl_avg_ctx_f_t *:   elib_fl_avg_update_f((ctx), (in)), \
        elib_fl_avg_ctx_q32_t *: elib_fl_avg_update_q32((ctx), (in)))

#define elib_fl_avg_reset(ctx) \
    _Generic((ctx), \
        elib_fl_avg_ctx_f_t *:   elib_fl_avg_reset_f(ctx), \
        elib_fl_avg_ctx_q32_t *: elib_fl_avg_reset_q32(ctx))

#ifdef __cplusplus
}
#endif

#endif /* ELIB_FL_AVG_H */
