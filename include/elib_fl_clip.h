/* elib_fl_clip.h - Amplitude Limiter Filter (C99) */

#ifndef ELIB_FL_CLIP_H
#define ELIB_FL_CLIP_H

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
    float out;
    float threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_clip_ctx_f_t;

elib_fl_err_t elib_fl_clip_init_f(elib_fl_clip_ctx_f_t *ctx, float threshold);
elib_fl_err_t elib_fl_clip_set_f(elib_fl_clip_ctx_f_t *ctx, float value);
float         elib_fl_clip_update_f(elib_fl_clip_ctx_f_t *ctx, float in);
void          elib_fl_clip_reset_f(elib_fl_clip_ctx_f_t *ctx);

/* ------------------------------------------------------------------ */
/*  Int32                                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    int32_t out;
    int32_t threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_clip_ctx_i32_t;

elib_fl_err_t elib_fl_clip_init_i32(elib_fl_clip_ctx_i32_t *ctx, int32_t threshold);
elib_fl_err_t elib_fl_clip_set_i32(elib_fl_clip_ctx_i32_t *ctx, int32_t value);
int32_t       elib_fl_clip_update_i32(elib_fl_clip_ctx_i32_t *ctx, int32_t in);
void          elib_fl_clip_reset_i32(elib_fl_clip_ctx_i32_t *ctx);

/* ------------------------------------------------------------------ */
/*  Uint32                                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t out;
    uint32_t threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_clip_ctx_u32_t;

elib_fl_err_t elib_fl_clip_init_u32(elib_fl_clip_ctx_u32_t *ctx, uint32_t threshold);
elib_fl_err_t elib_fl_clip_set_u32(elib_fl_clip_ctx_u32_t *ctx, uint32_t value);
uint32_t      elib_fl_clip_update_u32(elib_fl_clip_ctx_u32_t *ctx, uint32_t in);
void          elib_fl_clip_reset_u32(elib_fl_clip_ctx_u32_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ELIB_FL_CLIP_H */
