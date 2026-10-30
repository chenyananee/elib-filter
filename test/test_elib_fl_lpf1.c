/* test_elib_fl_lpf1.c - First-Order Low-Pass Filter Unit Tests (v2)
 *
 * All expectations below are hand-derived constants:
 *  - Q16 alpha = 16384 == 0.25 exactly (2^-2): every update is
 *    out += (diff * 16384) >> 16, an exact floor scalar product
 *  - float alpha = 0.25f: out_n = 100*(1 - 0.75^n), dyadic values
 *    with <= 24 mantissa bits, exact in IEEE single precision
 *  - the Qn rounding boundary case feeds alpha_f = 1 - 2^-23 through
 *    init_tau, whose (double) quantization rounds up to exactly 2^16
 *    -- the v2 guard must reject it (v1 silently stored alpha == 2^n,
 *    turning the filter into a pass-through)
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "elib_fl.h"

/* --- float init variants --- */

static void test_init_f_rejections(void) {
    printf("Test: init_f rejection matrix... ");
    elib_fl_lpf1_ctx_f_t ctx;
    elib_fl_err_t err;

    err = elib_fl_lpf1_init_f(NULL, 0.5f);
    assert(err == ELIB_FL_ERR_INVALID_PARAM);

    memset(&ctx, 0, sizeof(ctx));
    assert(elib_fl_lpf1_init_f(&ctx, 0.0f) == ELIB_FL_ERR_INVALID_PARAM);   /* alpha <= 0 */
    assert(elib_fl_lpf1_init_f(&ctx, -0.5f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_f(&ctx, 1.0f) == ELIB_FL_ERR_INVALID_PARAM);   /* alpha >= 1 */
    assert(elib_fl_lpf1_init_f(&ctx, 2.0f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);   /* rejection leaves ctx untouched */

    assert(elib_fl_lpf1_init_f(&ctx, 0.5f) == ELIB_FL_OK);
    assert(ctx.bit_flags.initialized == 1);
    assert(ctx.alpha == 0.5f);
    assert(ctx.out == 0.0f);

    printf("PASSED\n");
}

static void test_init_tau_f(void) {
    printf("Test: init_tau_f converts tau/dt... ");
    elib_fl_lpf1_ctx_f_t ctx;

    memset(&ctx, 0, sizeof(ctx));
    assert(elib_fl_lpf1_init_tau_f(&ctx, 0.0f, 0.01f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_tau_f(&ctx, 0.5f, 0.0f) == ELIB_FL_ERR_INVALID_PARAM);

    /* alpha = dt/(tau+dt) = 0.01/0.51 = 0.019607...; non-dyadic: the
     * compiler produces its correctly rounded float, we only assert
     * acceptance and the (0,1) invariant, never an exact bit value */
    assert(elib_fl_lpf1_init_tau_f(&ctx, 0.5f, 0.01f) == ELIB_FL_OK);
    assert(ctx.alpha > 0.0f && ctx.alpha < 1.0f);

    printf("PASSED\n");
}

static void test_init_fc_f(void) {
    printf("Test: init_fc_f converts fc/fs... ");
    elib_fl_lpf1_ctx_f_t ctx;

    memset(&ctx, 0, sizeof(ctx));
    assert(elib_fl_lpf1_init_fc_f(&ctx, 0.0f, 100.0f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_fc_f(&ctx, 10.0f, 0.0f) == ELIB_FL_ERR_INVALID_PARAM);

    assert(elib_fl_lpf1_init_fc_f(&ctx, 10.0f, 100.0f) == ELIB_FL_OK);
    /* true alpha = 0.01/(1/(20*pi*10) + 0.01) ~= 0.385865 */
    assert(ctx.alpha > 0.38f && ctx.alpha < 0.39f);

    printf("PASSED\n");
}

/* --- float update: exact dyadic sequence, alpha = 0.25f, in = 100 ---
 * out_n = 100*(1 - 0.75^n):
 *   25, 43.75, 57.8125, 68.359375, 76.26953125, 82.2021484375 ...
 * every value is a dyadic rational with <= 24 mantissa bits, so the
 * float arithmetic is EXACT and equality asserts are safe. */
static void test_update_f_step_exact(void) {
    printf("Test: update_f converges exactly (dyadic check)... ");
    elib_fl_lpf1_ctx_f_t ctx;

    assert(elib_fl_lpf1_init_f(&ctx, 0.25f) == ELIB_FL_OK);

    assert(elib_fl_lpf1_update_f(&ctx, 100.0f) == 25.0f);
    assert(elib_fl_lpf1_update_f(&ctx, 100.0f) == 43.75f);
    assert(elib_fl_lpf1_update_f(&ctx, 100.0f) == 57.8125f);
    assert(elib_fl_lpf1_update_f(&ctx, 100.0f) == 68.359375f);
    assert(elib_fl_lpf1_update_f(&ctx, 100.0f) == 76.26953125f);
    assert(elib_fl_lpf1_update_f(&ctx, 100.0f) == 82.2021484375f);

    /* descending, symmetric */
    elib_fl_lpf1_reset_f(&ctx);
    assert(elib_fl_lpf1_update_f(&ctx, -100.0f) == -25.0f);
    assert(elib_fl_lpf1_update_f(&ctx, -100.0f) == -43.75f);
    assert(elib_fl_lpf1_update_f(&ctx, -100.0f) == -57.8125f);

    /* in == out is a fixed point */
    elib_fl_lpf1_reset_f(&ctx);
    assert(elib_fl_lpf1_update_f(&ctx, 42.0f) == 10.5f);
    assert(elib_fl_lpf1_update_f(&ctx, 10.5f) == 10.5f);
    assert(elib_fl_lpf1_update_f(&ctx, 10.5f) == 10.5f);

    printf("PASSED\n");
}

static void test_reset_f(void) {
    printf("Test: reset_f re-zeros the state... ");
    elib_fl_lpf1_ctx_f_t ctx;

    elib_fl_lpf1_init_f(&ctx, 0.5f);
    (void)elib_fl_lpf1_update_f(&ctx, 100.0f);
    assert(ctx.out != 0.0f);

    elib_fl_lpf1_reset_f(&ctx);
    assert(ctx.out == 0.0f);
    assert(ctx.alpha == 0.5f);       /* config kept */
    assert(ctx.bit_flags.initialized == 1);

    /* first sample after reset behaves like fresh init */
    assert(elib_fl_lpf1_update_f(&ctx, 8.0f) == 4.0f);

    printf("PASSED\n");
}

static void test_uninitialized_f(void) {
    printf("Test: uninitialized float ctx returns the 0 sentinel... ");
    elib_fl_lpf1_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_update_f(&ctx, 100.0f) == 0.0f);
    assert(elib_fl_lpf1_update_f(NULL, 100.0f) == 0.0f);
    assert(ctx.out == 0.0f);         /* no state was moved */

    elib_fl_lpf1_reset_f(NULL);      /* tolerated no-op */
    elib_fl_lpf1_reset_f(&ctx);      /* uninitialized: no-op */

    printf("PASSED\n");
}

/* --- i32: Qn setup --- */

static void test_init_i32_rejections(void) {
    printf("Test: init_i32 rejection matrix... ");
    elib_fl_lpf1_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_init_i32(NULL, 16384, 16) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_i32(&ctx, 16384, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_i32(&ctx, 16384, 32) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_i32(&ctx, 0, 16) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_i32(&ctx, -1, 16) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_i32(&ctx, 65536, 16) == ELIB_FL_ERR_INVALID_PARAM);  /* alpha == 2^n */
    assert(elib_fl_lpf1_init_i32(&ctx, 70000, 16) == ELIB_FL_ERR_INVALID_PARAM);  /* alpha > 2^n */
    assert(ctx.bit_flags.initialized == 0);

    /* boundaries accepted: smallest (1) and largest (2^n - 1) */
    assert(elib_fl_lpf1_init_i32(&ctx, 1, 16) == ELIB_FL_OK);
    assert(elib_fl_lpf1_init_i32(&ctx, 65535, 16) == ELIB_FL_OK);
    assert(elib_fl_lpf1_init_i32(&ctx, (1 << 30), 31) == ELIB_FL_OK);

    printf("PASSED\n");
}

/* Q16 alpha = 16384 == 0.25, in = 100 (step), terminal value 97:
 *   out += floor(diff/4) each step:
 *   0 ->25 ->43 ->57 ->67 ->75 ->81 ->85 ->88 ->91 ->93 ->94 ->95
 *   ->96 ->97 (stuck: floor(3/4) == 0) */
static void test_update_i32_step_exact(void) {
    printf("Test: update_i32 exact floor-sequence (alpha=0.25 Q16)... ");
    elib_fl_lpf1_ctx_i32_t ctx;

    assert(elib_fl_lpf1_init_i32(&ctx, 16384, 16) == ELIB_FL_OK);

    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 25);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 43);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 57);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 67);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 75);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 81);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 85);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 88);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 91);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 93);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 94);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 95);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 96);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 97);
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 97);   /* floor(3/4)==0: parked */
    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 97);

    /* negative step: floor arithmetic visible (asymmetry vs float) */
    elib_fl_lpf1_reset_i32(&ctx);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -25);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -44);  /* floor(-18.75) */
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -58);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -69);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -77);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -83);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -88);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -91);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -94);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -96);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -97);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -98);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -99);
    assert(elib_fl_lpf1_update_i32(&ctx, -100) == -100); /* reached exactly */

    /* full-range input spikes never break the int64 pipeline */
    elib_fl_lpf1_reset_i32(&ctx);
    assert(elib_fl_lpf1_update_i32(&ctx, 2147483647) == 536870911);  /* (0.25)*(2^31-1) floor = 536870911 */
    elib_fl_lpf1_reset_i32(&ctx);
    assert(elib_fl_lpf1_update_i32(&ctx, -2147483647) == -536870912);
    /* diff = -(2^31-1): alpha*diff = -(2^29 - 0.25) -> floor = -536870912 */

    printf("PASSED\n");
}

static void test_init_tau_i32(void) {
    printf("Test: init_tau_i32 quantizes to Qn... ");
    elib_fl_lpf1_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_init_tau_i32(&ctx, 1.0f, 1.0f, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_tau_i32(&ctx, 1.0f, 1.0f, 32) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_tau_i32(&ctx, 0.0f, 1.0f, 16) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_tau_i32(&ctx, 1.0f, 0.0f, 16) == ELIB_FL_ERR_INVALID_PARAM);

    assert(elib_fl_lpf1_init_tau_i32(&ctx, 1.0f, 1.0f, 16) == ELIB_FL_OK);
    /* alpha_f = 0.5 -> scaled = 0.5*65536+0.5 = 32768.5 -> trunc 32768 */
    assert(ctx.alpha == 32768);
    assert(ctx.n == 16);

    printf("PASSED\n");
}

/* v2 rounding-boundary regression: tau ~ ulp(dt) yields
 * alpha_f = 1 - 2^-23. Derivation (IEEE single, round-to-nearest):
 *   d   = 1.0f; t = 1.5e-7f (nearest float ~1.49999997e-7)
 *   t+d = 1 + 1.49999997e-7, which rounds to 1 + 2^-23 (the two
 *         candidate neighbors are 1 and 1+2^-23; the gap favors it)
 *   a   = d/(t+d) = 1/(1+2^-23) = 1 - 2^-23 + 2^-46, which rounds
 *         to exactly 1 - 2^-23 (correctly rounded, nearest neighbor
 *         is 2^-46 away vs 2^-25 away)
 *   scaled = (double)(1-2^-23)*65536 + 0.5 = 65535.9921875+0.5
 *          = 65536.4921875 -> trunc = 65536 == 2^16
 * v1 stored alpha = 65536 (pass-through filter!); v2 must reject. */
static void test_init_tau_i32_rounding_boundary(void) {
    printf("Test: init_tau_i32 rejects quantized alpha == 2^n... ");
    elib_fl_lpf1_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_init_tau_i32(&ctx, 1.5e-7f, 1.0f, 16)
           == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);   /* nothing was stored */

    /* u32 sibling rejects the same boundary (it always did) */
    elib_fl_lpf1_ctx_u32_t uctx;
    memset(&uctx, 0, sizeof(uctx));
    assert(elib_fl_lpf1_init_tau_u32(&uctx, 1.5e-7f, 1.0f, 16)
           == ELIB_FL_ERR_INVALID_PARAM);
    assert(uctx.bit_flags.initialized == 0);

    printf("PASSED\n");
}

static void test_init_fc_i32(void) {
    printf("Test: init_fc_i32 quantizes to Qn... ");
    elib_fl_lpf1_ctx_i32_t ctx;

    memset(&ctx, 0, sizeof(ctx));
    assert(elib_fl_lpf1_init_fc_i32(&ctx, 10.0f, 100.0f, 16) == ELIB_FL_OK);
    /* alpha_f ~= 0.385865 -> stored alpha in (0.38,0.39)*65536, i.e.
     * strictly inside (0, 2^16); assert the invariant, not the bit
     * value (three correctly-rounded float divisions upstream) */
    assert(ctx.alpha > 25000 && ctx.alpha < 25500);
    assert(ctx.n == 16);

    assert(elib_fl_lpf1_init_fc_i32(&ctx, 10.0f, 100.0f, 0) == ELIB_FL_ERR_INVALID_PARAM);

    printf("PASSED\n");
}

static void test_uninitialized_i32(void) {
    printf("Test: uninitialized i32 ctx returns the 0 sentinel... ");
    elib_fl_lpf1_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_update_i32(&ctx, 100) == 0);
    assert(elib_fl_lpf1_update_i32(NULL, 100) == 0);
    assert(ctx.out == 0);

    printf("PASSED\n");
}

/* --- u32: mirror of i32 (non-negative domain) --- */

static void test_init_u32_rejections(void) {
    printf("Test: init_u32 rejection matrix... ");
    elib_fl_lpf1_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_init_u32(NULL, 16384, 16) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_u32(&ctx, 16384, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_u32(&ctx, 16384, 32) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_u32(&ctx, 0, 16) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_u32(&ctx, 65536, 16) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_lpf1_init_u32(&ctx, 16384, 16) == ELIB_FL_OK);

    printf("PASSED\n");
}

static void test_update_u32_step_exact(void) {
    printf("Test: update_u32 exact floor-sequence (alpha=0.25 Q16)... ");
    elib_fl_lpf1_ctx_u32_t ctx;

    assert(elib_fl_lpf1_init_u32(&ctx, 16384, 16) == ELIB_FL_OK);

    /* same positive-step ladder as i32 */
    assert(elib_fl_lpf1_update_u32(&ctx, 100) == 25);
    assert(elib_fl_lpf1_update_u32(&ctx, 100) == 43);
    assert(elib_fl_lpf1_update_u32(&ctx, 100) == 57);
    assert(elib_fl_lpf1_update_u32(&ctx, 100) == 67);
    assert(elib_fl_lpf1_update_u32(&ctx, 100) == 75);
    assert(elib_fl_lpf1_update_u32(&ctx, 100) == 81);
    assert(elib_fl_lpf1_update_u32(&ctx, 100) == 85);
    assert(elib_fl_lpf1_update_u32(&ctx, 100) == 88);
    assert(elib_fl_lpf1_update_u32(&ctx, 100) == 91);

    /* full uint32 range: diff < 0 handled by the int64 pipeline */
    elib_fl_lpf1_reset_u32(&ctx);
    assert(elib_fl_lpf1_update_u32(&ctx, 4000000000u) == 1000000000u);

    printf("PASSED\n");
}

static void test_uninitialized_u32(void) {
    printf("Test: uninitialized u32 ctx returns the 0 sentinel... ");
    elib_fl_lpf1_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_update_u32(&ctx, 100) == 0);
    assert(elib_fl_lpf1_update_u32(NULL, 100) == 0);

    printf("PASSED\n");
}

/* --- i16/u16: mirror flavours at half scale ---
 *
 * Q15 alpha = 16384 == 0.5 EXACTLY: every update is out +=
 * floor(diff / 2), and the int32 product is bound-provably safe
 * (16384 * 65535 = 1073741824 < 2^31). Hand-derived ladders:
 *   in = +100: 50, 75, 87, 93, 96, 98, 99, parked (diff 1 floors
 *     to 0 -- the +100 target is never reached)
 *   in = -100: -50, -75, -88, -94, -97, -99, -100, parked (floor
 *     pulls DOWN, so the negative side DOES reach -100 exactly)
 * The two directions are asymmetric by one step -- that is the
 * floor() bias, and it is the documented behaviour.
 */

static void test_init_i16_rejections(void) {
    printf("Test: init_i16 rejection matrix... ");
    elib_fl_lpf1_ctx_i16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_init_i16(NULL, 16384, 15) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_i16(&ctx, 16384, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_i16(&ctx, 16384, 16) == ELIB_FL_ERR_INVALID_PARAM);  /* n > 15 */
    assert(elib_fl_lpf1_init_i16(&ctx, 0, 15) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_i16(&ctx, -1, 15) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_i16(&ctx, 16384, 14) == ELIB_FL_ERR_INVALID_PARAM);  /* alpha == 2^14 */
    assert(ctx.bit_flags.initialized == 0);

    /* boundaries accepted: smallest and largest Q15 alphas */
    assert(elib_fl_lpf1_init_i16(&ctx, 1, 15) == ELIB_FL_OK);
    assert(elib_fl_lpf1_init_i16(&ctx, 32767, 15) == ELIB_FL_OK);   /* 0.99997 */
    assert(elib_fl_lpf1_init_i16(&ctx, 16384, 15) == ELIB_FL_OK);   /* 0.5 */

    printf("PASSED\n");
}

static void test_update_i16_step_exact(void) {
    printf("Test: update_i16 exact floor-ladder (alpha=0.5 Q15)... ");
    elib_fl_lpf1_ctx_i16_t ctx;

    assert(elib_fl_lpf1_init_i16(&ctx, 16384, 15) == ELIB_FL_OK);

    /* out += floor(diff/2), ascending (diff = 100 - out) */
    assert(elib_fl_lpf1_update_i16(&ctx, 100) == 50);    /* floor(100/2) */
    assert(elib_fl_lpf1_update_i16(&ctx, 100) == 75);    /* floor(50/2)  */
    assert(elib_fl_lpf1_update_i16(&ctx, 100) == 87);    /* floor(25/2)  */
    assert(elib_fl_lpf1_update_i16(&ctx, 100) == 93);    /* floor(13/2)  */
    assert(elib_fl_lpf1_update_i16(&ctx, 100) == 96);    /* floor(7/2)   */
    assert(elib_fl_lpf1_update_i16(&ctx, 100) == 98);    /* floor(4/2)   */
    assert(elib_fl_lpf1_update_i16(&ctx, 100) == 99);    /* floor(2/2)   */
    assert(elib_fl_lpf1_update_i16(&ctx, 100) == 99);    /* floor(1/2)=0: parked */
    assert(elib_fl_lpf1_update_i16(&ctx, 100) == 99);

    /* descending: floor bias reaches the target exactly */
    elib_fl_lpf1_reset_i16(&ctx);
    assert(elib_fl_lpf1_update_i16(&ctx, -100) == -50);
    assert(elib_fl_lpf1_update_i16(&ctx, -100) == -75);  /* floor(-50/2) */
    assert(elib_fl_lpf1_update_i16(&ctx, -100) == -88);  /* floor(-25/2) = -13 */
    assert(elib_fl_lpf1_update_i16(&ctx, -100) == -94);  /* floor(-12/2) */
    assert(elib_fl_lpf1_update_i16(&ctx, -100) == -97);  /* floor(-6/2)  */
    assert(elib_fl_lpf1_update_i16(&ctx, -100) == -99);  /* floor(-3/2) = -2 */
    assert(elib_fl_lpf1_update_i16(&ctx, -100) == -100); /* floor(-1/2) = -1 */
    assert(elib_fl_lpf1_update_i16(&ctx, -100) == -100);

    /* full int16 range probes */
    elib_fl_lpf1_reset_i16(&ctx);
    assert(elib_fl_lpf1_update_i16(&ctx, 32767) == 16383);   /* floor(32767/2) */
    elib_fl_lpf1_reset_i16(&ctx);
    assert(elib_fl_lpf1_update_i16(&ctx, -32768) == -16384); /* exact half */

    printf("PASSED\n");
}

static void test_init_tau_i16(void) {
    printf("Test: init_tau_i16 quantizes to Qn (n<=15)... ");
    elib_fl_lpf1_ctx_i16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_init_tau_i16(&ctx, 1.0f, 1.0f, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_tau_i16(&ctx, 1.0f, 1.0f, 16) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_tau_i16(&ctx, 0.0f, 1.0f, 15) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_tau_i16(&ctx, 1.0f, 0.0f, 15) == ELIB_FL_ERR_INVALID_PARAM);

    assert(elib_fl_lpf1_init_tau_i16(&ctx, 1.0f, 1.0f, 15) == ELIB_FL_OK);
    /* alpha_f = 0.5 -> scaled = 0.5*32768 + 0.5 = 16384.5 -> trunc 16384 */
    assert(ctx.alpha == 16384);
    assert(ctx.n == 15);

    printf("PASSED\n");
}

/* v2 rounding-boundary regression at Q15: alpha_f = 1 - 2^-23 (same
 * IEEE derivation as the Q16 case) times 2^15 plus 0.5 gives
 * 32767.996 + 0.5 = 32768.496 -> trunc = 32768 == 2^15 -- must be
 * rejected, not stored (near-pass-through). */
static void test_init_tau_i16_rounding_boundary(void) {
    printf("Test: init_tau_i16 rejects quantized alpha == 2^15... ");
    elib_fl_lpf1_ctx_i16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_init_tau_i16(&ctx, 1.5e-7f, 1.0f, 15)
           == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    /* u16 sibling rejects the same boundary */
    elib_fl_lpf1_ctx_u16_t uctx;
    memset(&uctx, 0, sizeof(uctx));
    assert(elib_fl_lpf1_init_tau_u16(&uctx, 1.5e-7f, 1.0f, 15)
           == ELIB_FL_ERR_INVALID_PARAM);
    assert(uctx.bit_flags.initialized == 0);

    printf("PASSED\n");
}

static void test_init_fc_i16(void) {
    printf("Test: init_fc_i16 quantizes to Qn... ");
    elib_fl_lpf1_ctx_i16_t ctx;

    memset(&ctx, 0, sizeof(ctx));
    assert(elib_fl_lpf1_init_fc_i16(&ctx, 10.0f, 100.0f, 15) == ELIB_FL_OK);
    /* alpha_f ~= 0.385865 -> scaled ~= 0.385865 * 32768 = 12650.8,
     * +0.5 trunc -> 12651; assert a loose window, not the bit value */
    assert(ctx.alpha > 12600 && ctx.alpha < 12700);
    assert(ctx.n == 15);

    assert(elib_fl_lpf1_init_fc_i16(&ctx, 10.0f, 100.0f, 0) == ELIB_FL_ERR_INVALID_PARAM);

    printf("PASSED\n");
}

static void test_uninitialized_i16(void) {
    printf("Test: uninitialized i16 ctx returns the 0 sentinel... ");
    elib_fl_lpf1_ctx_i16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_update_i16(&ctx, 100) == 0);
    assert(elib_fl_lpf1_update_i16(NULL, 100) == 0);
    assert(ctx.out == 0);

    printf("PASSED\n");
}

static void test_init_u16_rejections(void) {
    printf("Test: init_u16 rejection matrix... ");
    elib_fl_lpf1_ctx_u16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_init_u16(NULL, 16384, 15) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_u16(&ctx, 16384, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_u16(&ctx, 16384, 16) == ELIB_FL_ERR_INVALID_PARAM);  /* n > 15 */
    assert(elib_fl_lpf1_init_u16(&ctx, 0, 15) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_lpf1_init_u16(&ctx, 32768, 15) == ELIB_FL_ERR_INVALID_PARAM);  /* alpha == 2^15 */
    assert(elib_fl_lpf1_init_u16(&ctx, 40000, 15) == ELIB_FL_ERR_INVALID_PARAM);  /* alpha > 2^15 */
    assert(ctx.bit_flags.initialized == 0);

    /* boundary accepted: alpha 32767 == 2^15 - 1 (0.99997) */
    assert(elib_fl_lpf1_init_u16(&ctx, 32767, 15) == ELIB_FL_OK);
    assert(elib_fl_lpf1_init_u16(&ctx, 16384, 15) == ELIB_FL_OK);

    printf("PASSED\n");
}

static void test_update_u16_step_exact(void) {
    printf("Test: update_u16 exact floor-ladder (alpha=0.5 Q15)... ");
    elib_fl_lpf1_ctx_u16_t ctx;

    assert(elib_fl_lpf1_init_u16(&ctx, 16384, 15) == ELIB_FL_OK);

    /* same ascending ladder as i16 (floor(diff/2)) */
    assert(elib_fl_lpf1_update_u16(&ctx, 100) == 50);
    assert(elib_fl_lpf1_update_u16(&ctx, 100) == 75);
    assert(elib_fl_lpf1_update_u16(&ctx, 100) == 87);
    assert(elib_fl_lpf1_update_u16(&ctx, 100) == 93);
    assert(elib_fl_lpf1_update_u16(&ctx, 100) == 96);
    assert(elib_fl_lpf1_update_u16(&ctx, 100) == 98);
    assert(elib_fl_lpf1_update_u16(&ctx, 100) == 99);
    assert(elib_fl_lpf1_update_u16(&ctx, 100) == 99);

    /* full uint16 range, descending dyadic halving (each step exact
     * or floored by the odd 0.5 residue; from 32767 down it is a
     * clean power-of-two staircase: 16384, 8192, 4096, 2048 ...) */
    elib_fl_lpf1_reset_u16(&ctx);
    assert(elib_fl_lpf1_update_u16(&ctx, 65535) == 32767);   /* floor(65535/2) */
    assert(elib_fl_lpf1_update_u16(&ctx, 0) == 16384);       /* floor(-32767/2) = -16383 */
    assert(elib_fl_lpf1_update_u16(&ctx, 0) == 8192);        /* floor(-16384/2) */
    assert(elib_fl_lpf1_update_u16(&ctx, 0) == 4096);
    assert(elib_fl_lpf1_update_u16(&ctx, 0) == 2048);

    printf("PASSED\n");
}

static void test_uninitialized_u16(void) {
    printf("Test: uninitialized u16 ctx returns the 0 sentinel... ");
    elib_fl_lpf1_ctx_u16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_lpf1_update_u16(&ctx, 100) == 0);
    assert(elib_fl_lpf1_update_u16(NULL, 100) == 0);

    printf("PASSED\n");
}

int main(void) {
    printf("=== elib-filter (lpf1) tests ===\n\n");

    test_init_f_rejections();
    test_init_tau_f();
    test_init_fc_f();
    test_update_f_step_exact();
    test_reset_f();
    test_uninitialized_f();

    test_init_i32_rejections();
    test_update_i32_step_exact();
    test_init_tau_i32();
    test_init_tau_i32_rounding_boundary();
    test_init_fc_i32();
    test_uninitialized_i32();

    test_init_u32_rejections();
    test_update_u32_step_exact();
    test_uninitialized_u32();

    test_init_i16_rejections();
    test_update_i16_step_exact();
    test_init_tau_i16();
    test_init_tau_i16_rounding_boundary();
    test_init_fc_i16();
    test_uninitialized_i16();

    test_init_u16_rejections();
    test_update_u16_step_exact();
    test_uninitialized_u16();

    printf("\n=== All tests passed ===\n");
    return 0;
}
