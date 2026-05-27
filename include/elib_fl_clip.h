/* elib_fl_clip.h - Amplitude Limiter Filter */

#ifndef ELIB_FL_CLIP_H
#define ELIB_FL_CLIP_H

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
    float    threshold;
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_clip_ctx_f_t;

elib_fl_err_t elib_fl_clip_init_f(elib_fl_clip_ctx_f_t *ctx, float threshold);
elib_fl_err_t elib_fl_clip_set_f(elib_fl_clip_ctx_f_t *ctx, float value);
float         elib_fl_clip_update_f(elib_fl_clip_ctx_f_t *ctx, float in);
float         elib_fl_clip_oneshot_f(float threshold, float in, float prev);
void          elib_fl_clip_reset_f(elib_fl_clip_ctx_f_t *ctx);

/* ------------------------------------------------------------------ */
/*  Q32 context                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    int32_t  out;
    int32_t  threshold;    /* Qn format */
    uint8_t  n;            /* fractional bits */
    struct {
        uint8_t initialized : 1;
    } bit_flags;
} elib_fl_clip_ctx_q32_t;

elib_fl_err_t elib_fl_clip_init_q32(elib_fl_clip_ctx_q32_t *ctx, int32_t threshold, uint8_t n);
elib_fl_err_t elib_fl_clip_set_q32(elib_fl_clip_ctx_q32_t *ctx, int32_t value);
int32_t       elib_fl_clip_update_q32(elib_fl_clip_ctx_q32_t *ctx, int32_t in);
int32_t       elib_fl_clip_oneshot_q32(int32_t threshold, int32_t in, int32_t prev);
void          elib_fl_clip_reset_q32(elib_fl_clip_ctx_q32_t *ctx);

/* ------------------------------------------------------------------ */
/*  Generic API                                                        */
/* ------------------------------------------------------------------ */

#define elib_fl_clip_init(ctx, ...) \
    _Generic((ctx), \
        elib_fl_clip_ctx_f_t *:   elib_fl_clip_init_f((ctx), __VA_ARGS__), \
        elib_fl_clip_ctx_q32_t *: elib_fl_clip_init_q32((ctx), __VA_ARGS__))

#define elib_fl_clip_set(ctx, ...) \
    _Generic((ctx), \
        elib_fl_clip_ctx_f_t *:   elib_fl_clip_set_f((ctx), __VA_ARGS__), \
        elib_fl_clip_ctx_q32_t *: elib_fl_clip_set_q32((ctx), __VA_ARGS__))

#define elib_fl_clip_update(ctx, in) \
    _Generic((ctx), \
        elib_fl_clip_ctx_f_t *:   elib_fl_clip_update_f((ctx), (in)), \
        elib_fl_clip_ctx_q32_t *: elib_fl_clip_update_q32((ctx), (in)))

#define elib_fl_clip_reset(ctx) \
    _Generic((ctx), \
        elib_fl_clip_ctx_f_t *:   elib_fl_clip_reset_f(ctx), \
        elib_fl_clip_ctx_q32_t *: elib_fl_clip_reset_q32(ctx))

#ifdef __cplusplus
}
#endif

#endif /* ELIB_FL_CLIP_H */
