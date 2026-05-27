/* elib_fl_mavg.h - Moving Average Filter */

#ifndef ELIB_FL_MAVG_H
#define ELIB_FL_MAVG_H

#include "elib_fl_err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Float context                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    float    *buf;
    float     sum;
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_mavg_ctx_f_t;

elib_fl_err_t elib_fl_mavg_init_f(elib_fl_mavg_ctx_f_t *ctx, float *buf, uint32_t size);
float         elib_fl_mavg_update_f(elib_fl_mavg_ctx_f_t *ctx, float in);
float         elib_fl_mavg_oneshot_f(const float *buf, uint32_t size);
uint32_t      elib_fl_mavg_warmup_f(const elib_fl_mavg_ctx_f_t *ctx);
void          elib_fl_mavg_reset_f(elib_fl_mavg_ctx_f_t *ctx);

/* ------------------------------------------------------------------ */
/*  Q32 context                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    int32_t  *buf;
    int32_t   sum;         /* running sum, max = size * INT32_MAX */
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_mavg_ctx_q32_t;

elib_fl_err_t elib_fl_mavg_init_q32(elib_fl_mavg_ctx_q32_t *ctx, int32_t *buf, uint32_t size);
int32_t       elib_fl_mavg_update_q32(elib_fl_mavg_ctx_q32_t *ctx, int32_t in);
int32_t       elib_fl_mavg_oneshot_q32(const int32_t *buf, uint32_t size);
uint32_t      elib_fl_mavg_warmup_q32(const elib_fl_mavg_ctx_q32_t *ctx);
void          elib_fl_mavg_reset_q32(elib_fl_mavg_ctx_q32_t *ctx);

/* ------------------------------------------------------------------ */
/*  Generic API                                                        */
/* ------------------------------------------------------------------ */

#define elib_fl_mavg_init(ctx, ...) \
    _Generic((ctx), \
        elib_fl_mavg_ctx_f_t *:   elib_fl_mavg_init_f((ctx), __VA_ARGS__), \
        elib_fl_mavg_ctx_q32_t *: elib_fl_mavg_init_q32((ctx), __VA_ARGS__))

#define elib_fl_mavg_update(ctx, in) \
    _Generic((ctx), \
        elib_fl_mavg_ctx_f_t *:   elib_fl_mavg_update_f((ctx), (in)), \
        elib_fl_mavg_ctx_q32_t *: elib_fl_mavg_update_q32((ctx), (in)))

#define elib_fl_mavg_warmup(ctx) \
    _Generic((ctx), \
        elib_fl_mavg_ctx_f_t *:   elib_fl_mavg_warmup_f(ctx), \
        elib_fl_mavg_ctx_q32_t *: elib_fl_mavg_warmup_q32(ctx))

#define elib_fl_mavg_reset(ctx) \
    _Generic((ctx), \
        elib_fl_mavg_ctx_f_t *:   elib_fl_mavg_reset_f(ctx), \
        elib_fl_mavg_ctx_q32_t *: elib_fl_mavg_reset_q32(ctx))

#ifdef __cplusplus
}
#endif

#endif /* ELIB_FL_MAVG_H */
