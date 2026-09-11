/* elib_fl_lpf1.h - First-Order Low-Pass Filter (C99) */

#ifndef ELIB_FL_LPF1_H
#define ELIB_FL_LPF1_H

#include "elib_fl_err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Float                                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    float   out;
    float   alpha;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_lpf1_ctx_f_t;

elib_fl_err_t elib_fl_lpf1_init_f(elib_fl_lpf1_ctx_f_t *ctx, float alpha);
elib_fl_err_t elib_fl_lpf1_init_tau_f(elib_fl_lpf1_ctx_f_t *ctx, float tau, float dt);
elib_fl_err_t elib_fl_lpf1_init_fc_f(elib_fl_lpf1_ctx_f_t *ctx, float fc, float fs);
float         elib_fl_lpf1_update_f(elib_fl_lpf1_ctx_f_t *ctx, float in);
void          elib_fl_lpf1_reset_f(elib_fl_lpf1_ctx_f_t *ctx);

/* ------------------------------------------------------------------ */
/*  Int32 (alpha in Qn fixed-point)                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    int32_t out;
    int32_t alpha;
    uint8_t n;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_lpf1_ctx_i32_t;

elib_fl_err_t elib_fl_lpf1_init_i32(elib_fl_lpf1_ctx_i32_t *ctx, int32_t alpha, uint8_t n);
elib_fl_err_t elib_fl_lpf1_init_tau_i32(elib_fl_lpf1_ctx_i32_t *ctx, float tau, float dt, uint8_t n);
elib_fl_err_t elib_fl_lpf1_init_fc_i32(elib_fl_lpf1_ctx_i32_t *ctx, float fc, float fs, uint8_t n);
int32_t       elib_fl_lpf1_update_i32(elib_fl_lpf1_ctx_i32_t *ctx, int32_t in);
void          elib_fl_lpf1_reset_i32(elib_fl_lpf1_ctx_i32_t *ctx);

/* ------------------------------------------------------------------ */
/*  Uint32 (alpha in Qn fixed-point)                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t out;
    uint32_t alpha;
    uint8_t  n;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_lpf1_ctx_u32_t;

elib_fl_err_t elib_fl_lpf1_init_u32(elib_fl_lpf1_ctx_u32_t *ctx, uint32_t alpha, uint8_t n);
elib_fl_err_t elib_fl_lpf1_init_tau_u32(elib_fl_lpf1_ctx_u32_t *ctx, float tau, float dt, uint8_t n);
elib_fl_err_t elib_fl_lpf1_init_fc_u32(elib_fl_lpf1_ctx_u32_t *ctx, float fc, float fs, uint8_t n);
uint32_t      elib_fl_lpf1_update_u32(elib_fl_lpf1_ctx_u32_t *ctx, uint32_t in);
void          elib_fl_lpf1_reset_u32(elib_fl_lpf1_ctx_u32_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ELIB_FL_LPF1_H */
