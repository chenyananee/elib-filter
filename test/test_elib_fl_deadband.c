/* test_elib_fl_deadband.c - Dead-Zone Filter Unit Tests (v2)
 *
 * All expectations are hand-derived constants. Semantics:
 *   |in - out| <= threshold -> hold out   (inclusive dead zone)
 *   |in - out| >  threshold -> follow in  (one past the edge moves)
 * Mirror image of clip: small steps HELD, large steps FOLLOWED.
 * The full-range probes exercise the int32/int64 diff pipelines.
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "elib_fl.h"

/* --- float --- */

static void test_init_f(void) {
    printf("Test: deadband init_f rejection matrix... ");
    elib_fl_deadband_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_deadband_init_f(NULL, 5.0f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_deadband_init_f(&ctx, 0.0f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_deadband_init_f(&ctx, -5.0f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_deadband_init_f(&ctx, 5.0f) == ELIB_FL_OK);
    assert(ctx.threshold == 5.0f);
    assert(ctx.out == 0.0f);
    assert(ctx.bit_flags.initialized == 1);

    printf("PASSED\n");
}

/* ladder: state 100, threshold 5 (dead zone inclusive) */
static void test_update_f_ladder(void) {
    printf("Test: deadband update_f hold/follow ladder... ");
    elib_fl_deadband_ctx_f_t ctx;

    assert(elib_fl_deadband_init_f(&ctx, 5.0f) == ELIB_FL_OK);
    assert(elib_fl_deadband_set_f(&ctx, 100.0f) == ELIB_FL_OK);

    assert(elib_fl_deadband_update_f(&ctx, 102.0f) == 100.0f);  /* diff 2: held */
    assert(elib_fl_deadband_update_f(&ctx, 98.0f) == 100.0f);   /* diff 2: held */
    assert(elib_fl_deadband_update_f(&ctx, 103.0f) == 100.0f);  /* diff 3: held */
    assert(elib_fl_deadband_update_f(&ctx, 106.0f) == 106.0f);  /* diff 6: followed */
    assert(elib_fl_deadband_update_f(&ctx, 111.0f) == 106.0f);  /* diff 5 == threshold: held */
    assert(elib_fl_deadband_update_f(&ctx, 112.0f) == 112.0f);  /* diff 6: followed */

    /* huge step follows immediately (clip would hold it) */
    assert(elib_fl_deadband_update_f(&ctx, -1000.0f) == -1000.0f);
    assert(elib_fl_deadband_update_f(&ctx, -995.0f) == -1000.0f);   /* diff 5: held */
    assert(elib_fl_deadband_update_f(&ctx, -994.0f) == -994.0f);    /* diff 6: followed */

    printf("PASSED\n");
}

/* static-jitter scenario: references 105, noise +/-3 stays frozen */
static void test_static_jitter_f(void) {
    printf("Test: deadband update_f static jitter frozen... ");
    elib_fl_deadband_ctx_f_t ctx;

    assert(elib_fl_deadband_init_f(&ctx, 5.0f) == ELIB_FL_OK);
    assert(elib_fl_deadband_set_f(&ctx, 105.0f) == ELIB_FL_OK);

    assert(elib_fl_deadband_update_f(&ctx, 108.0f) == 105.0f);  /* +3 */
    assert(elib_fl_deadband_update_f(&ctx, 101.0f) == 105.0f);  /* -4 */
    assert(elib_fl_deadband_update_f(&ctx, 107.0f) == 105.0f);  /* +2 */
    assert(elib_fl_deadband_update_f(&ctx, 105.0f) == 105.0f);  /* 0 */
    assert(elib_fl_deadband_update_f(&ctx, 110.0f) == 110.0f);  /* +5 == threshold: still held */
    assert(elib_fl_deadband_update_f(&ctx, 111.0f) == 111.0f);  /* +1 past: follows */

    printf("PASSED\n");
}

static void test_reset_first_sample_f(void) {
    printf("Test: deadband reset/first-sample behaviour... ");
    elib_fl_deadband_ctx_f_t ctx;

    assert(elib_fl_deadband_init_f(&ctx, 5.0f) == ELIB_FL_OK);
    (void)elib_fl_deadband_set_f(&ctx, 42.0f);

    elib_fl_deadband_reset_f(&ctx);
    assert(ctx.out == 0.0f);
    assert(ctx.threshold == 5.0f);        /* config kept */

    /* from state 0: small input stays inside the dead zone around 0 */
    assert(elib_fl_deadband_update_f(&ctx, 3.0f) == 0.0f);
    assert(elib_fl_deadband_update_f(&ctx, -4.0f) == 0.0f);
    assert(elib_fl_deadband_update_f(&ctx, 6.0f) == 6.0f);

    printf("PASSED\n");
}

static void test_uninitialized_f(void) {
    printf("Test: deadband uninitialized float behaviour... ");
    elib_fl_deadband_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_deadband_set_f(&ctx, 1.0f) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_deadband_update_f(&ctx, 100.0f) == 0.0f);
    assert(elib_fl_deadband_update_f(NULL, 100.0f) == 0.0f);
    assert(ctx.out == 0.0f);

    elib_fl_deadband_reset_f(NULL);       /* tolerated no-ops */
    elib_fl_deadband_reset_f(&ctx);

    printf("PASSED\n");
}

/* --- i16 --- */

static void test_i16_ladder_and_full_range(void) {
    printf("Test: deadband i16 ladder + full-range int32 diff... ");
    elib_fl_deadband_ctx_i16_t ctx;

    assert(elib_fl_deadband_init_i16(&ctx, 5) == ELIB_FL_OK);
    assert(elib_fl_deadband_set_i16(&ctx, 100) == ELIB_FL_OK);

    assert(elib_fl_deadband_update_i16(&ctx, 102) == 100);   /* diff 2: held */
    assert(elib_fl_deadband_update_i16(&ctx, 106) == 106);   /* diff 6: followed */
    assert(elib_fl_deadband_update_i16(&ctx, 112) == 112);   /* diff 6: followed */

    /* threshold INT16_MAX: |diff| = 2^16-1 fits int32 easily */
    assert(elib_fl_deadband_init_i16(&ctx, INT16_MAX) == ELIB_FL_OK);
    assert(elib_fl_deadband_set_i16(&ctx, 0) == ELIB_FL_OK);
    assert(elib_fl_deadband_update_i16(&ctx, 32767) == 0);           /* diff 32767 == threshold: held */
    assert(elib_fl_deadband_set_i16(&ctx, 0) == ELIB_FL_OK);
    assert(elib_fl_deadband_update_i16(&ctx, -32768) == -32768);     /* diff 32768 > threshold: followed */

    assert(elib_fl_deadband_set_i16(&ctx, -32768) == ELIB_FL_OK);
    assert(elib_fl_deadband_update_i16(&ctx, 32767) == 32767);       /* diff 65535: followed */

    printf("PASSED\n");
}

static void test_i16_uninitialized(void) {
    printf("Test: deadband uninitialized i16 behaviour... ");
    elib_fl_deadband_ctx_i16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_deadband_set_i16(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_deadband_update_i16(&ctx, 100) == 0);

    printf("PASSED\n");
}

/* --- i32 --- */

static void test_init_i32(void) {
    printf("Test: deadband init_i32 rejection matrix... ");
    elib_fl_deadband_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_deadband_init_i32(NULL, 5) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_deadband_init_i32(&ctx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_deadband_init_i32(&ctx, -5) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_deadband_init_i32(&ctx, 5) == ELIB_FL_OK);

    printf("PASSED\n");
}

static void test_i32_full_range(void) {
    printf("Test: deadband i32 full-range int64 diff... ");
    elib_fl_deadband_ctx_i32_t ctx;

    assert(elib_fl_deadband_init_i32(&ctx, 5) == ELIB_FL_OK);
    assert(elib_fl_deadband_set_i32(&ctx, INT32_MIN) == ELIB_FL_OK);

    assert(elib_fl_deadband_update_i32(&ctx, INT32_MIN) == INT32_MIN);  /* diff 0: held */
    assert(elib_fl_deadband_update_i32(&ctx, INT32_MAX) == INT32_MAX);  /* diff 2^32-2: followed (int64) */
    assert(elib_fl_deadband_update_i32(&ctx, 2147483640) == INT32_MAX); /* diff -7: held */

    printf("PASSED\n");
}

static void test_i32_uninitialized(void) {
    printf("Test: deadband uninitialized i32 behaviour... ");
    elib_fl_deadband_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_deadband_set_i32(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_deadband_update_i32(&ctx, 100) == 0);

    printf("PASSED\n");
}

/* --- u16 --- */

static void test_u16_ladder_and_full_range(void) {
    printf("Test: deadband u16 ladder + full-range int32 diff... ");
    elib_fl_deadband_ctx_u16_t ctx;

    assert(elib_fl_deadband_init_u16(&ctx, 5) == ELIB_FL_OK);
    assert(elib_fl_deadband_set_u16(&ctx, 100) == ELIB_FL_OK);

    assert(elib_fl_deadband_update_u16(&ctx, 104) == 100);      /* diff 4: held */
    assert(elib_fl_deadband_update_u16(&ctx, 105) == 100);      /* diff 5 == threshold: held */
    assert(elib_fl_deadband_update_u16(&ctx, 106) == 106);      /* diff 6: followed */

    assert(elib_fl_deadband_set_u16(&ctx, 0) == ELIB_FL_OK);
    assert(elib_fl_deadband_update_u16(&ctx, 65535u) == 65535u); /* diff 65535: followed */
    assert(elib_fl_deadband_set_u16(&ctx, 65535u) == ELIB_FL_OK);
    assert(elib_fl_deadband_update_u16(&ctx, 0) == 0);           /* diff -65535: followed */

    printf("PASSED\n");
}

static void test_u16_uninitialized(void) {
    printf("Test: deadband uninitialized u16 behaviour... ");
    elib_fl_deadband_ctx_u16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_deadband_set_u16(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_deadband_update_u16(&ctx, 100) == 0);

    printf("PASSED\n");
}

/* --- u32 --- */

static void test_init_u32(void) {
    printf("Test: deadband init_u32 rejection matrix... ");
    elib_fl_deadband_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_deadband_init_u32(NULL, 5) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_deadband_init_u32(&ctx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_deadband_init_u32(&ctx, 5) == ELIB_FL_OK);

    printf("PASSED\n");
}

static void test_u32_full_range(void) {
    printf("Test: deadband u32 full-range int64 diff... ");
    elib_fl_deadband_ctx_u32_t ctx;

    assert(elib_fl_deadband_init_u32(&ctx, 5) == ELIB_FL_OK);
    assert(elib_fl_deadband_set_u32(&ctx, 0) == ELIB_FL_OK);

    assert(elib_fl_deadband_update_u32(&ctx, 4294967295u) == 4294967295u); /* follow */
    assert(elib_fl_deadband_update_u32(&ctx, 0) == 0);                     /* diff -2^32+1: follow */
    assert(elib_fl_deadband_update_u32(&ctx, 3) == 0);                     /* diff 3: held */

    printf("PASSED\n");
}

static void test_u32_uninitialized(void) {
    printf("Test: deadband uninitialized u32 behaviour... ");
    elib_fl_deadband_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_deadband_set_u32(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_deadband_update_u32(&ctx, 100) == 0);

    printf("PASSED\n");
}

int main(void) {
    printf("=== elib-filter (deadband) tests ===\n\n");

    test_init_f();
    test_update_f_ladder();
    test_static_jitter_f();
    test_reset_first_sample_f();
    test_uninitialized_f();

    test_i16_ladder_and_full_range();
    test_i16_uninitialized();

    test_init_i32();
    test_i32_full_range();
    test_i32_uninitialized();

    test_u16_ladder_and_full_range();
    test_u16_uninitialized();

    test_init_u32();
    test_u32_full_range();
    test_u32_uninitialized();

    printf("\n=== All tests passed ===\n");
    return 0;
}
