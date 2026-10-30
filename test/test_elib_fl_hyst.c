/* test_elib_fl_hyst.c - Boundary Hysteresis Limiter Unit Tests (v3)
 *
 * All expectations are hand-derived constants. Reference anchors use
 * boundary b = 5, hysteresis h = 2, snap S = 3:
 *
 *   normal up   1,2,3,4,6,7,8   -> 1,2,3,4,5,5,8   (sticky at b)
 *   jump up     0,6             -> 0,6             (snap through)
 *   jump down   10,5            -> 10,5            (snap through)
 *   hold down   7,6,5,4,3       -> 7,6,6,6,3       (hold band, honest at -h)
 *   delta == S NEVER breaks the hysteresis (strict `>`)
 *
 * The temperature scenario (fan-threshold 35 C, h = 2, S = 5) walks
 * gradual rise / sticky clamp / cross / decay hold / fall snap and
 * both snap directions. int64 pipeline is probed with full-range
 * int32/uint32 hops.
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "elib_fl.h"

/* --- float: the anchor state machine (b=5, h=2, S=3) --- */

static void test_init_f(void) {
    printf("Test: hyst init_f rejection matrix... ");
    elib_fl_hyst_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_hyst_init_f(NULL, 5.0f, 2.0f, 3.0f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 0.0f, 3.0f) == ELIB_FL_ERR_INVALID_PARAM);   /* h = 0 */
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, -2.0f, 3.0f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 2.0f) == ELIB_FL_ERR_INVALID_PARAM);   /* S == h */
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 1.0f) == ELIB_FL_ERR_INVALID_PARAM);   /* S < h */
    assert(elib_fl_hyst_init_f(&ctx, 0.0f / 0.0f, 2.0f, 3.0f) == ELIB_FL_ERR_INVALID_PARAM); /* NaN b */
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 0.0f / 0.0f, 3.0f) == ELIB_FL_ERR_INVALID_PARAM); /* NaN h */
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 0.0f / 0.0f) == ELIB_FL_ERR_INVALID_PARAM); /* NaN S */
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    assert(ctx.out == 0.0f);
    assert(ctx.boundary == 5.0f);
    assert(ctx.hyst == 2.0f);
    assert(ctx.snap == 3.0f);
    assert(ctx.bit_flags.initialized == 1);
    assert(ctx.bit_flags.above == 0);

    printf("PASSED\n");
}

static void test_up_walk_f(void) {
    printf("Test: hyst update_f normal up walk (sticky clamp)... ");
    elib_fl_hyst_ctx_f_t ctx;

    /* LOW side: actual feed up to the boundary */
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    assert(elib_fl_hyst_update_f(&ctx, 1.0f) == 1.0f);
    assert(elib_fl_hyst_update_f(&ctx, 2.0f) == 2.0f);
    assert(elib_fl_hyst_update_f(&ctx, 3.0f) == 3.0f);
    assert(elib_fl_hyst_update_f(&ctx, 4.0f) == 4.0f);
    /* sticky band (0, h]: clamped to the boundary, never above */
    assert(elib_fl_hyst_update_f(&ctx, 6.0f) == 5.0f);
    assert(elib_fl_hyst_update_f(&ctx, 7.0f) == 5.0f);
    assert(elib_fl_hyst_update_f(&ctx, 5.0f) == 5.0f);   /* s == 0: actual anyway */
    /* cross the envelope: honest, HIGH */
    assert(elib_fl_hyst_update_f(&ctx, 8.0f) == 8.0f);
    assert(ctx.bit_flags.above == 1);

    printf("PASSED\n");
}

static void test_down_walk_f(void) {
    printf("Test: hyst update_f hold-band decay walk... ");
    elib_fl_hyst_ctx_f_t ctx;

    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    (void)elib_fl_hyst_update_f(&ctx, 8.0f);   /* enter HIGH at 8 */
    assert(elib_fl_hyst_update_f(&ctx, 7.0f) == 7.0f);  /* s > 0: actual */
    assert(elib_fl_hyst_update_f(&ctx, 6.0f) == 6.0f);  /* s > 0: actual */
    assert(elib_fl_hyst_update_f(&ctx, 5.0f) == 6.0f);  /* s == 0, d 1 <= S: hold */
    assert(elib_fl_hyst_update_f(&ctx, 4.0f) == 6.0f);  /* -h < s < 0, d 2 <= S: hold */
    assert(elib_fl_hyst_update_f(&ctx, 3.0f) == 3.0f);  /* s == -h: honest, LOW */
    assert(ctx.bit_flags.above == 0);
    /* LOW side below the boundary: actual all the way */
    assert(elib_fl_hyst_update_f(&ctx, 2.0f) == 2.0f);
    assert(elib_fl_hyst_update_f(&ctx, 0.0f) == 0.0f);
    assert(elib_fl_hyst_update_f(&ctx, -4.0f) == -4.0f);

    printf("PASSED\n");
}

static void test_snap_f(void) {
    printf("Test: hyst update_f snap-through both directions... ");
    elib_fl_hyst_ctx_f_t ctx;

    /* jump up 0 -> 6: |6 - 0| = 6 > 3 snaps through the sticky band */
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    assert(elib_fl_hyst_update_f(&ctx, 0.0f) == 0.0f);
    assert(elib_fl_hyst_update_f(&ctx, 6.0f) == 6.0f);   /* snap, not clamp */
    assert(ctx.bit_flags.above == 1);
    /* above the boundary HIGH rules take over: honest on the way up,
     * then the hold band catches the return */
    assert(elib_fl_hyst_update_f(&ctx, 7.0f) == 7.0f);   /* s > 0: actual */
    assert(elib_fl_hyst_update_f(&ctx, 5.0f) == 7.0f);   /* d = 2 <= 3: hold */
    assert(elib_fl_hyst_update_f(&ctx, 4.0f) == 7.0f);   /* d = 3 == S: hold */
    assert(elib_fl_hyst_update_f(&ctx, 3.0f) == 3.0f);   /* s = -h: honest LOW */
    assert(ctx.bit_flags.above == 0);

    /* jump down 10 -> 5: |5 - 10| = 5 > 3 breaks the hold band.
     * in == boundary lands on the LOW side. */
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    (void)elib_fl_hyst_update_f(&ctx, 10.0f);            /* 0 -> s=5 > h: HIGH */
    assert(elib_fl_hyst_update_f(&ctx, 10.0f) == 10.0f); /* hold out=10 */
    assert(elib_fl_hyst_update_f(&ctx, 5.0f) == 5.0f);   /* snap down, LOW */
    assert(ctx.bit_flags.above == 0);

    printf("PASSED\n");
}

static void test_snap_boundary_f(void) {
    printf("Test: hyst update_f d == snap never breaks... ");
    elib_fl_hyst_ctx_f_t ctx;

    /* LOW sticky: d == S clamps (strict >), stays LOW */
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    (void)elib_fl_hyst_update_f(&ctx, 4.0f);
    assert(elib_fl_hyst_update_f(&ctx, 7.0f) == 5.0f);   /* d = 3 == S: clamp */
    assert(ctx.bit_flags.above == 0);

    /* HIGH hold: d == S holds the high value */
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    (void)elib_fl_hyst_update_f(&ctx, 8.0f);
    assert(elib_fl_hyst_update_f(&ctx, 5.0f) == 8.0f);   /* d = 3 == S: hold */
    assert(ctx.bit_flags.above == 1);

    /* huge honest cross from LOW is a cross (not snap), same output */
    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    assert(elib_fl_hyst_update_f(&ctx, 30.0f) == 30.0f); /* s = 25 > h */
    assert(ctx.bit_flags.above == 1);

    printf("PASSED\n");
}

static void test_set_f(void) {
    printf("Test: hyst set_f seed derives the side... ");
    elib_fl_hyst_ctx_f_t ctx;

    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    assert(elib_fl_hyst_set_f(&ctx, 7.0f) == ELIB_FL_OK);  /* above: HIGH */
    assert(ctx.out == 7.0f);
    assert(ctx.bit_flags.above == 1);
    assert(elib_fl_hyst_update_f(&ctx, 4.0f) == 7.0f);  /* d = 3 == S: hold */
    assert(elib_fl_hyst_update_f(&ctx, 3.0f) == 3.0f);  /* s = -h: LOW, actual */

    memset(&ctx, 0, sizeof(ctx));
    (void)elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f);
    assert(elib_fl_hyst_set_f(&ctx, 4.0f) == ELIB_FL_OK);  /* below: LOW */
    assert(ctx.bit_flags.above == 0);
    assert(elib_fl_hyst_update_f(&ctx, 4.0f) == 4.0f);

    printf("PASSED\n");
}

static void test_reset_f(void) {
    printf("Test: hyst reset_f backs to LOW out of the gate... ");
    elib_fl_hyst_ctx_f_t ctx;

    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    (void)elib_fl_hyst_update_f(&ctx, 8.0f);
    assert(ctx.bit_flags.above == 1);

    elib_fl_hyst_reset_f(&ctx);
    assert(ctx.out == 0.0f);
    assert(ctx.bit_flags.above == 0);
    assert(ctx.boundary == 5.0f);       /* config kept */
    assert(ctx.hyst == 2.0f);
    assert(ctx.snap == 3.0f);
    assert(ctx.bit_flags.initialized == 1);

    /* back in LOW, out = 0: small negative step is actual */
    assert(elib_fl_hyst_update_f(&ctx, -2.0f) == -2.0f);

    printf("PASSED\n");
}

static void test_nan_f(void) {
    printf("Test: hyst update_f NaN swallowed, output held... ");
    elib_fl_hyst_ctx_f_t ctx;

    assert(elib_fl_hyst_init_f(&ctx, 5.0f, 2.0f, 3.0f) == ELIB_FL_OK);
    (void)elib_fl_hyst_update_f(&ctx, 8.0f);          /* HIGH, out = 8 */

    assert(elib_fl_hyst_update_f(&ctx, 0.0f / 0.0f) == 8.0f);  /* NaN: held */
    assert(ctx.bit_flags.above == 1);                  /* state untouched */
    assert(elib_fl_hyst_update_f(&ctx, 3.0f) == 3.0f); /* next real sample: honest */

    printf("PASSED\n");
}

static void test_uninitialized_f(void) {
    printf("Test: hyst uninitialized float behaviour... ");
    elib_fl_hyst_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_hyst_set_f(&ctx, 1.0f) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_hyst_update_f(&ctx, 100.0f) == 0.0f);
    assert(elib_fl_hyst_update_f(NULL, 100.0f) == 0.0f);
    assert(ctx.out == 0.0f);

    elib_fl_hyst_reset_f(NULL);       /* tolerated no-ops */
    elib_fl_hyst_reset_f(&ctx);

    printf("PASSED\n");
}

/* --- float: temperature scenario (fan threshold 35 C) --- */

static void test_temperature_f(void) {
    printf("Test: hyst update_f temperature-grade scenario (35/2/5)... ");
    elib_fl_hyst_ctx_f_t ctx;

    /* fan-on threshold boundary = 35 C, hysteresis 2 C, snap 5 C.
     * Sensor warms up gradually from room temperature. */
    assert(elib_fl_hyst_init_f(&ctx, 35.0f, 2.0f, 5.0f) == ELIB_FL_OK);
    assert(elib_fl_hyst_set_f(&ctx, 30.0f) == ELIB_FL_OK);   /* start at 30, LOW */

    /* gradual rise: actual until 34, sticky at the boundary */
    assert(elib_fl_hyst_update_f(&ctx, 31.0f) == 31.0f);
    assert(elib_fl_hyst_update_f(&ctx, 32.0f) == 32.0f);
    assert(elib_fl_hyst_update_f(&ctx, 33.0f) == 33.0f);
    assert(elib_fl_hyst_update_f(&ctx, 34.0f) == 34.0f);
    assert(elib_fl_hyst_update_f(&ctx, 36.0f) == 35.0f);   /* sticky clamp */
    assert(elib_fl_hyst_update_f(&ctx, 37.0f) == 35.0f);   /* sticky clamp */
    /* thermal runaway: 39 breaks the envelope honestly */
    assert(elib_fl_hyst_update_f(&ctx, 39.0f) == 39.0f);
    assert(ctx.bit_flags.above == 1);

    /* slow decay: honest while still above the boundary */
    assert(elib_fl_hyst_update_f(&ctx, 38.0f) == 38.0f);
    assert(elib_fl_hyst_update_f(&ctx, 37.0f) == 37.0f);
    /* every in <= 33 sits on or below the -h edge: deep fall wins
     * before the hold band can pick it up */
    assert(elib_fl_hyst_update_f(&ctx, 34.0f) == 37.0f);   /* d = 3 <= 5: hold */
    assert(elib_fl_hyst_update_f(&ctx, 33.0f) == 33.0f);   /* s = -2 == -h: deep, honest LOW */
    assert(ctx.bit_flags.above == 0);
    assert(elib_fl_hyst_update_f(&ctx, 31.0f) == 31.0f);   /* deep side: actual */

    /* re-cross honest, then exact-edge fall */
    assert(elib_fl_hyst_update_f(&ctx, 38.0f) == 38.0f);   /* s = 3 > h: HIGH */
    assert(ctx.bit_flags.above == 1);
    assert(elib_fl_hyst_update_f(&ctx, 33.0f) == 33.0f);   /* s = -2 == -h: LOW */
    assert(ctx.bit_flags.above == 0);

    /* upward snap from well below: out = 33, in = 36 -> |3| <= 5: clamp */
    assert(elib_fl_hyst_update_f(&ctx, 36.0f) == 35.0f);
    /* jump 29 -> 36: |7| > 5 breaks straight through */
    assert(elib_fl_hyst_update_f(&ctx, 29.0f) == 29.0f);
    assert(elib_fl_hyst_update_f(&ctx, 36.0f) == 36.0f);   /* snap up HIGH */
    assert(ctx.bit_flags.above == 1);
    /* full-h deep drop is honest immediately: 36 -> 30, s = -5 <= -h */
    assert(elib_fl_hyst_update_f(&ctx, 30.0f) == 30.0f);
    assert(ctx.bit_flags.above == 0);

    printf("PASSED\n");
}

/* --- i16 --- */

static void test_i16_anchor(void) {
    printf("Test: hyst i16 anchor walk + rejection matrix... ");
    elib_fl_hyst_ctx_i16_t ctx;

    assert(elib_fl_hyst_init_i16(NULL, 5, 2, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_i16(&ctx, 5, 0, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_i16(&ctx, 5, 2, 2) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_i16(&ctx, 5, 2, 1) == ELIB_FL_ERR_INVALID_PARAM);

    assert(elib_fl_hyst_init_i16(&ctx, 5, 2, 3) == ELIB_FL_OK);
    assert(elib_fl_hyst_set_i16(&ctx, 4) == ELIB_FL_OK);
    assert(elib_fl_hyst_update_i16(&ctx, 4) == 4);
    assert(elib_fl_hyst_update_i16(&ctx, 6) == 5);   /* sticky */
    assert(elib_fl_hyst_update_i16(&ctx, 7) == 5);   /* sticky */
    assert(elib_fl_hyst_update_i16(&ctx, 8) == 8);   /* cross HIGH */
    assert(elib_fl_hyst_update_i16(&ctx, 6) == 6);   /* s > 0: actual */
    assert(elib_fl_hyst_update_i16(&ctx, 5) == 6);   /* hold */
    assert(elib_fl_hyst_update_i16(&ctx, 4) == 6);   /* hold */
    assert(elib_fl_hyst_update_i16(&ctx, 3) == 3);   /* s == -h: LOW */

    /* negative boundary + negative walk: the jump -2 -> 7 breaks the
     * sticky band (|9| > 3) and lands HIGH above the boundary */
    assert(elib_fl_hyst_set_i16(&ctx, -4) == ELIB_FL_OK);   /* b = 5: LOW */
    assert(elib_fl_hyst_update_i16(&ctx, -2) == -2);        /* s = -7: actual */
    assert(elib_fl_hyst_update_i16(&ctx, 7) == 7);          /* s = 2, d = 9 > 3: snap HIGH */
    assert(elib_fl_hyst_update_i16(&ctx, 9) == 9);          /* s = 4 > 0: actual */

    printf("PASSED\n");
}

static void test_i16_full_range(void) {
    printf("Test: hyst i16 full-range int32 diff... ");
    elib_fl_hyst_ctx_i16_t ctx;

    assert(elib_fl_hyst_init_i16(&ctx, 0, 1, 2) == ELIB_FL_OK);
    assert(elib_fl_hyst_update_i16(&ctx, 32767) == 32767);   /* s = 2^15-1 > h */
    assert(elib_fl_hyst_update_i16(&ctx, -32768) == -32768); /* s very negative */
    assert(elib_fl_hyst_update_i16(&ctx, 0) == 0);
    /* S == h rejects even at the band-edge value */
    assert(elib_fl_hyst_init_i16(&ctx, 0, 32767, 32767) == ELIB_FL_ERR_INVALID_PARAM);

    assert(elib_fl_hyst_init_i16(&ctx, 0, 10000, 30000) == ELIB_FL_OK);
    assert(elib_fl_hyst_update_i16(&ctx, 30000) == 30000);   /* cross: s = 30000 */
    assert(elib_fl_hyst_update_i16(&ctx, 20001) == 20001);   /* s > 0: actual */
    assert(elib_fl_hyst_update_i16(&ctx, 10001) == 10001);   /* s > 0: actual */
    assert(elib_fl_hyst_update_i16(&ctx, 10000) == 10000);   /* s > 0: actual (else cross) */
    assert(elib_fl_hyst_update_i16(&ctx, 0) == 10000);       /* s = 0 hold: d 10000 <= S */
    assert(elib_fl_hyst_update_i16(&ctx, -1) == 10000);      /* hold: d 10001 */
    assert(elib_fl_hyst_update_i16(&ctx, -9999) == 10000);   /* s = -9999 > -h: hold, d 19999 */
    assert(elib_fl_hyst_update_i16(&ctx, -10000) == -10000); /* s = -h: honest LOW */

    printf("PASSED\n");
}

static void test_i16_uninitialized(void) {
    printf("Test: hyst uninitialized i16 behaviour... ");
    elib_fl_hyst_ctx_i16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_hyst_set_i16(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_hyst_update_i16(&ctx, 100) == 0);

    printf("PASSED\n");
}

/* --- i32 --- */

static void test_i32_anchor_and_reject(void) {
    printf("Test: hyst i32 anchor walk + rejection matrix... ");
    elib_fl_hyst_ctx_i32_t ctx;

    assert(elib_fl_hyst_init_i32(NULL, 5, 2, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_i32(&ctx, 5, 0, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_i32(&ctx, 5, 2, 2) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_i32(&ctx, 5, 2, 1) == ELIB_FL_ERR_INVALID_PARAM);

    assert(elib_fl_hyst_init_i32(&ctx, 5, 2, 3) == ELIB_FL_OK);
    assert(elib_fl_hyst_set_i32(&ctx, 4) == ELIB_FL_OK);
    assert(elib_fl_hyst_update_i32(&ctx, 6) == 5);   /* sticky */
    assert(elib_fl_hyst_update_i32(&ctx, 7) == 5);   /* sticky */
    assert(elib_fl_hyst_update_i32(&ctx, 8) == 8);   /* cross */
    /* snap down: |5 - 8| = 3 == S does NOT break; 4 does */
    assert(elib_fl_hyst_update_i32(&ctx, 5) == 8);
    assert(elib_fl_hyst_update_i32(&ctx, 4) == 4);
    assert(ctx.bit_flags.above == 0);

    printf("PASSED\n");
}

static void test_i32_full_range(void) {
    printf("Test: hyst i32 full-range int64 diff... ");
    elib_fl_hyst_ctx_i32_t ctx;

    /* hop across the entire int32 range: only int64 survives */
    assert(elib_fl_hyst_init_i32(&ctx, 0, 1, 2) == ELIB_FL_OK);
    assert(elib_fl_hyst_update_i32(&ctx, INT32_MAX) == INT32_MAX);   /* cross HIGH */
    assert(elib_fl_hyst_update_i32(&ctx, INT32_MIN) == INT32_MIN);   /* s == -2^31 <= -h */
    assert(ctx.bit_flags.above == 0);

    /* wide band: everything between stays honest while above */
    assert(elib_fl_hyst_init_i32(&ctx, 0, 1000000000, 2000000000) == ELIB_FL_OK);
    assert(elib_fl_hyst_set_i32(&ctx, INT32_MAX) == ELIB_FL_OK);     /* HIGH */
    assert(elib_fl_hyst_update_i32(&ctx, 2000000000) == 2000000000); /* s > 0 */
    assert(elib_fl_hyst_update_i32(&ctx, 0) == 2000000000);          /* s = 0 hold */
    assert(elib_fl_hyst_update_i32(&ctx, INT32_MIN) == INT32_MIN);   /* deep fall */
    assert(ctx.bit_flags.above == 0);

    printf("PASSED\n");
}

static void test_i32_uninitialized(void) {
    printf("Test: hyst uninitialized i32 behaviour... ");
    elib_fl_hyst_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_hyst_set_i32(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_hyst_update_i32(&ctx, 100) == 0);

    printf("PASSED\n");
}

/* --- u16 --- */

static void test_u16_anchor(void) {
    printf("Test: hyst u16 anchor walk + rejection matrix... ");
    elib_fl_hyst_ctx_u16_t ctx;

    assert(elib_fl_hyst_init_u16(NULL, 100u, 10u, 20u) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_u16(&ctx, 100u, 0u, 20u) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_u16(&ctx, 100u, 10u, 10u) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_u16(&ctx, 100u, 10u, 5u) == ELIB_FL_ERR_INVALID_PARAM);

    assert(elib_fl_hyst_init_u16(&ctx, 100u, 10u, 20u) == ELIB_FL_OK);
    assert(elib_fl_hyst_set_u16(&ctx, 90u) == ELIB_FL_OK);       /* LOW */
    assert(elib_fl_hyst_update_u16(&ctx, 95u) == 95u);     /* s = -5: actual */
    assert(elib_fl_hyst_update_u16(&ctx, 105u) == 100u);   /* sticky clamp */
    assert(elib_fl_hyst_update_u16(&ctx, 110u) == 100u);   /* sticky clamp */
    assert(elib_fl_hyst_update_u16(&ctx, 111u) == 111u);   /* s = 11 > h: HIGH */
    assert(elib_fl_hyst_update_u16(&ctx, 105u) == 105u);   /* s > 0: actual */
    assert(elib_fl_hyst_update_u16(&ctx, 100u) == 105u);   /* hold, d = 5 */
    assert(elib_fl_hyst_update_u16(&ctx, 95u) == 105u);    /* hold, d = 10 */
    assert(elib_fl_hyst_update_u16(&ctx, 90u) == 90u);     /* s = -10: LOW */

    printf("PASSED\n");
}

static void test_u16_full_range(void) {
    printf("Test: hyst u16 full-range int32 diff... ");
    elib_fl_hyst_ctx_u16_t ctx;

    /* b = 0, h = 1, snap = 2 */
    assert(elib_fl_hyst_init_u16(&ctx, 0u, 1u, 2u) == ELIB_FL_OK);
    assert(elib_fl_hyst_update_u16(&ctx, 65535u) == 65535u); /* s = 65535 > h: cross HIGH */
    assert(elib_fl_hyst_update_u16(&ctx, 0u) == 0u);         /* s = 0 hold band, d 65535 > 2: snap, LOW */
    assert(elib_fl_hyst_update_u16(&ctx, 1u) == 0u);         /* s = 1 sticky, d = 1 <= 2: clamp to b */
    assert(elib_fl_hyst_update_u16(&ctx, 3u) == 3u);         /* s = 3 > h: cross HIGH */
    assert(elib_fl_hyst_update_u16(&ctx, 2u) == 2u);         /* s = 2 > 0: actual */
    assert(elib_fl_hyst_update_u16(&ctx, 0u) == 2u);         /* s = 0 hold band, d = 2 <= 2: hold */

    printf("PASSED\n");
}

static void test_u16_uninitialized(void) {
    printf("Test: hyst uninitialized u16 behaviour... ");
    elib_fl_hyst_ctx_u16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_hyst_set_u16(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_hyst_update_u16(&ctx, 100) == 0);

    printf("PASSED\n");
}

/* --- u32 --- */

static void test_u32_full_range(void) {
    printf("Test: hyst u32 full-range int64 diff... ");
    elib_fl_hyst_ctx_u32_t ctx;

    assert(elib_fl_hyst_init_u32(NULL, 100u, 5u, 10u) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_u32(&ctx, 100u, 0u, 10u) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_hyst_init_u32(&ctx, 100u, 5u, 5u) == ELIB_FL_ERR_INVALID_PARAM);

    /* 0 <-> 2^32-1 hops: seamless on the int64 pipeline */
    assert(elib_fl_hyst_init_u32(&ctx, 100u, 5u, 10u) == ELIB_FL_OK);
    assert(elib_fl_hyst_update_u32(&ctx, 4294967295u) == 4294967295u); /* cross HIGH */
    assert(elib_fl_hyst_update_u32(&ctx, 4294967200u) == 4294967200u); /* s = 4294967100: honest */
    assert(elib_fl_hyst_update_u32(&ctx, 0u) == 0u);                   /* s deep negative: LOW */
    assert(elib_fl_hyst_update_u32(&ctx, 95u) == 95u);                 /* s = -5: actual */
    assert(elib_fl_hyst_update_u32(&ctx, 3u) == 3u);                   /* s = -97: actual */

    printf("PASSED\n");
}

static void test_u32_uninitialized(void) {
    printf("Test: hyst uninitialized u32 behaviour... ");
    elib_fl_hyst_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_hyst_set_u32(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_hyst_update_u32(&ctx, 100) == 0);

    printf("PASSED\n");
}

int main(void) {
    printf("=== elib-filter (hyst) tests ===\n\n");

    test_init_f();
    test_up_walk_f();
    test_down_walk_f();
    test_snap_f();
    test_snap_boundary_f();
    test_set_f();
    test_reset_f();
    test_nan_f();
    test_uninitialized_f();
    test_temperature_f();

    test_i16_anchor();
    test_i16_full_range();
    test_i16_uninitialized();

    test_i32_anchor_and_reject();
    test_i32_full_range();
    test_i32_uninitialized();

    test_u16_anchor();
    test_u16_full_range();
    test_u16_uninitialized();

    test_u32_full_range();
    test_u32_uninitialized();

    printf("\n=== All tests passed ===\n");
    return 0;
}
