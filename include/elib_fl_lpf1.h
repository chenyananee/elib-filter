/* elib_fl_lpf1.h - First-Order Low-Pass Filter */

#ifndef ELIB_FL_LPF1_H
#define ELIB_FL_LPF1_H

#include "elib_fl_err.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Float context                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    float    out;
    float    alpha;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_lpf1_ctx_f_t;

elib_fl_err_t elib_fl_lpf1_init_f(elib_fl_lpf1_ctx_f_t *ctx, float alpha);
elib_fl_err_t elib_fl_lpf1_init_tau_f(elib_fl_lpf1_ctx_f_t *ctx, float tau, float dt);
elib_fl_err_t elib_fl_lpf1_init_fc_f(elib_fl_lpf1_ctx_f_t *ctx, float fc, float fs);
float         elib_fl_lpf1_update_f(elib_fl_lpf1_ctx_f_t *ctx, float in);
float         elib_fl_lpf1_oneshot_f(float alpha, float in, float prev);
void          elib_fl_lpf1_reset_f(elib_fl_lpf1_ctx_f_t *ctx);

/* ------------------------------------------------------------------ */
/*  Q32 context                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    int32_t  out;
    int32_t  alpha;        /* Qn format */
    uint8_t  n;            /* fractional bits */
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_lpf1_ctx_q32_t;

elib_fl_err_t elib_fl_lpf1_init_q32(elib_fl_lpf1_ctx_q32_t *ctx, int32_t alpha, uint8_t n);
elib_fl_err_t elib_fl_lpf1_init_tau_q32(elib_fl_lpf1_ctx_q32_t *ctx, float tau, float dt, uint8_t n);
elib_fl_err_t elib_fl_lpf1_init_fc_q32(elib_fl_lpf1_ctx_q32_t *ctx, float fc, float fs, uint8_t n);
int32_t       elib_fl_lpf1_update_q32(elib_fl_lpf1_ctx_q32_t *ctx, int32_t in);
int32_t       elib_fl_lpf1_oneshot_q32(int32_t alpha, int32_t in, int32_t prev, uint8_t n);
void          elib_fl_lpf1_reset_q32(elib_fl_lpf1_ctx_q32_t *ctx);

/* ------------------------------------------------------------------ */
/*  Generic API                                                        */
/* ------------------------------------------------------------------ */

#define elib_fl_lpf1_init(ctx, ...) \
    _Generic((ctx), \
        elib_fl_lpf1_ctx_f_t *:   elib_fl_lpf1_init_f((ctx), __VA_ARGS__), \
        elib_fl_lpf1_ctx_q32_t *: elib_fl_lpf1_init_q32((ctx), __VA_ARGS__))

#define elib_fl_lpf1_update(ctx, in) \
    _Generic((ctx), \
        elib_fl_lpf1_ctx_f_t *:   elib_fl_lpf1_update_f((ctx), (in)), \
        elib_fl_lpf1_ctx_q32_t *: elib_fl_lpf1_update_q32((ctx), (in)))

#define elib_fl_lpf1_reset(ctx) \
    _Generic((ctx), \
        elib_fl_lpf1_ctx_f_t *:   elib_fl_lpf1_reset_f(ctx), \
        elib_fl_lpf1_ctx_q32_t *: elib_fl_lpf1_reset_q32(ctx))

#ifdef __cplusplus
}
#endif

#endif /* ELIB_FL_LPF1_H */
