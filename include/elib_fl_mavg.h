/* elib_fl_mavg.h - Moving Average Filter (C99) */

#ifndef ELIB_FL_MAVG_H
#define ELIB_FL_MAVG_H

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
uint32_t      elib_fl_mavg_warmup_f(const elib_fl_mavg_ctx_f_t *ctx);
void          elib_fl_mavg_reset_f(elib_fl_mavg_ctx_f_t *ctx);

/* ------------------------------------------------------------------ */
/*  Int32                                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    int32_t  *buf;
    int32_t   sum;
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_mavg_ctx_i32_t;

elib_fl_err_t elib_fl_mavg_init_i32(elib_fl_mavg_ctx_i32_t *ctx, int32_t *buf, uint32_t size);
int32_t       elib_fl_mavg_update_i32(elib_fl_mavg_ctx_i32_t *ctx, int32_t in);
uint32_t      elib_fl_mavg_warmup_i32(const elib_fl_mavg_ctx_i32_t *ctx);
void          elib_fl_mavg_reset_i32(elib_fl_mavg_ctx_i32_t *ctx);

/* ------------------------------------------------------------------ */
/*  Uint32                                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t *buf;
    uint32_t  sum;
    uint32_t  size;
    uint32_t  idx;
    uint32_t  count;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_mavg_ctx_u32_t;

elib_fl_err_t elib_fl_mavg_init_u32(elib_fl_mavg_ctx_u32_t *ctx, uint32_t *buf, uint32_t size);
uint32_t      elib_fl_mavg_update_u32(elib_fl_mavg_ctx_u32_t *ctx, uint32_t in);
uint32_t      elib_fl_mavg_warmup_u32(const elib_fl_mavg_ctx_u32_t *ctx);
void          elib_fl_mavg_reset_u32(elib_fl_mavg_ctx_u32_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ELIB_FL_MAVG_H */
