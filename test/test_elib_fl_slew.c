/* test_elib_fl_slew.c - Slew-Rate Limiter Unit Tests (v3)
 *
 * All expectations are hand-derived integer constants:
 *  - threshold 5, initial state 100: 104 accepted (diff 4),
 *    200/-1000 rejected (diff 96/1104), exactly-at-threshold steps
 *    (diff == 5) are ACCEPTED (the update is `<= threshold`)
 *  - the i32/u32 full-range cases drive |in - out| to 2^32-2 /
 *    2^31-sized magnitudes, which only the int64 diff pipeline
 *    survives (an int32 diff would overflow / be UB)
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "elib_fl.h"

/* --- float --- */

static void test_init_f(void) {
    printf("Test: slew init_f rejection matrix... ");
    elib_fl_slew_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_slew_init_f(NULL, 5.0f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_slew_init_f(&ctx, 0.0f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_slew_init_f(&ctx, -5.0f) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_slew_init_f(&ctx, 5.0f) == ELIB_FL_OK);
    assert(ctx.threshold == 5.0f);
    assert(ctx.out == 0.0f);
    assert(ctx.bit_flags.initialized == 1);

    printf("PASSED\n");
}

/* ladder: state 100, threshold 5 */
static void test_update_f_ladder(void) {
    printf("Test: slew update_f accept/reject ladder... ");
    elib_fl_slew_ctx_f_t ctx;

    assert(elib_fl_slew_init_f(&ctx, 5.0f) == ELIB_FL_OK);
    assert(elib_fl_slew_set_f(&ctx, 100.0f) == ELIB_FL_OK);

    (void)elib_fl_slew_update_f(&ctx, 104.0f);
    assert(ctx.out == 104.0f);            /* diff 4 <= 5: accepted */

    assert(elib_fl_slew_update_f(&ctx, 200.0f) == 104.0f);   /* diff 96: held */
    assert(elib_fl_slew_update_f(&ctx, -1000.0f) == 104.0f); /* diff 1104: held */

    assert(elib_fl_slew_update_f(&ctx, 99.0f) == 99.0f);     /* diff 5 == threshold: accepted */
    assert(elib_fl_slew_update_f(&ctx, 94.0f) == 94.0f);     /* diff 5 (down): accepted */

    /* within-threshold drift is followed */
    assert(elib_fl_slew_update_f(&ctx, 94.5f) == 94.5f);

    printf("PASSED\n");
}

static void test_set_reset_f(void) {
    printf("Test: slew set/reset f... ");
    elib_fl_slew_ctx_f_t ctx;

    assert(elib_fl_slew_init_f(&ctx, 5.0f) == ELIB_FL_OK);
    assert(elib_fl_slew_set_f(&ctx, 42.0f) == ELIB_FL_OK);
    assert(ctx.out == 42.0f);

    elib_fl_slew_reset_f(&ctx);
    assert(ctx.out == 0.0f);
    assert(ctx.threshold == 5.0f);        /* config kept */
    assert(ctx.bit_flags.initialized == 1);

    /* reset re-opens the limiter around 0 */
    assert(elib_fl_slew_update_f(&ctx, 3.0f) == 3.0f);

    printf("PASSED\n");
}

static void test_uninitialized_f(void) {
    printf("Test: slew uninitialized float behaviour... ");
    elib_fl_slew_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_slew_set_f(&ctx, 1.0f) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_slew_update_f(&ctx, 100.0f) == 0.0f);
    assert(elib_fl_slew_update_f(NULL, 100.0f) == 0.0f);
    assert(ctx.out == 0.0f);              /* nothing moved */

    elib_fl_slew_reset_f(NULL);           /* tolerated no-ops */
    elib_fl_slew_reset_f(&ctx);

    printf("PASSED\n");
}

/* --- i32 --- */

static void test_init_i32(void) {
    printf("Test: slew init_i32 rejection matrix... ");
    elib_fl_slew_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_slew_init_i32(NULL, 5) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_slew_init_i32(&ctx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_slew_init_i32(&ctx, -5) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_slew_init_i32(&ctx, 5) == ELIB_FL_OK);
    assert(ctx.threshold == 5);

    printf("PASSED\n");
}

/* threshold INT32_MAX: the two extreme probes bracket the whole
 * int32 range. diff is computed in int64:
 *   update(INT32_MAX)  from out=0: diff = 2147483647 <= threshold: accept
 *   update(INT32_MIN)  from out=INT32_MAX: diff = -4294967294: reject
 * (in 32-bit arithmetic both steps would be UB / wrap) */
static void test_update_i32_full_range(void) {
    printf("Test: slew update_i32 full-range int64 diff... ");
    elib_fl_slew_ctx_i32_t ctx;

    assert(elib_fl_slew_init_i32(&ctx, INT32_MAX) == ELIB_FL_OK);

    assert(elib_fl_slew_update_i32(&ctx, INT32_MAX) == INT32_MAX);
    assert(elib_fl_slew_update_i32(&ctx, INT32_MIN) == INT32_MAX);  /* held */

    /* boundary from 0 down to INT32_MIN: |diff| = 2^31 -> one past
     * the threshold, rejected (negating it in int32 would be UB) */
    assert(elib_fl_slew_set_i32(&ctx, 0) == ELIB_FL_OK);
    assert(elib_fl_slew_update_i32(&ctx, INT32_MIN) == 0);

    printf("PASSED\n");
}

static void test_update_i32_ladder(void) {
    printf("Test: slew update_i32 accept/reject ladder... ");
    elib_fl_slew_ctx_i32_t ctx;

    assert(elib_fl_slew_init_i32(&ctx, 5) == ELIB_FL_OK);
    assert(elib_fl_slew_set_i32(&ctx, 100) == ELIB_FL_OK);

    assert(elib_fl_slew_update_i32(&ctx, 104) == 104);        /* diff 4 */
    assert(elib_fl_slew_update_i32(&ctx, 200) == 104);        /* diff 96: held */
    assert(elib_fl_slew_update_i32(&ctx, -1000) == 104);      /* diff 1104: held */
    assert(elib_fl_slew_update_i32(&ctx, 99) == 99);          /* diff 5: accepted */
    assert(elib_fl_slew_update_i32(&ctx, 94) == 94);          /* diff -5: accepted */

    printf("PASSED\n");
}

static void test_set_uninitialized_i32(void) {
    printf("Test: slew uninitialized i32 behaviour... ");
    elib_fl_slew_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_slew_set_i32(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_slew_set_i32(NULL, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_slew_update_i32(&ctx, 100) == 0);
    assert(ctx.out == 0);

    printf("PASSED\n");
}

/* --- u32 --- */

static void test_init_u32(void) {
    printf("Test: slew init_u32 rejection matrix... ");
    elib_fl_slew_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_slew_init_u32(NULL, 5) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_slew_init_u32(&ctx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_slew_init_u32(&ctx, 5) == ELIB_FL_OK);
    assert(ctx.threshold == 5);

    printf("PASSED\n");
}

/* threshold 5, full-range probes: state 4294967295 (2^32-1):
 *   update(0)            -> diff = -4294967295: reject, hold
 *   update(4294967290)   -> diff = -5: accept (boundary) */
static void test_update_u32_full_range(void) {
    printf("Test: slew update_u32 full-range int64 diff... ");
    elib_fl_slew_ctx_u32_t ctx;

    assert(elib_fl_slew_init_u32(&ctx, 5) == ELIB_FL_OK);
    assert(elib_fl_slew_set_u32(&ctx, 4294967295u) == ELIB_FL_OK);

    assert(elib_fl_slew_update_u32(&ctx, 0) == 4294967295u);          /* held */
    assert(elib_fl_slew_update_u32(&ctx, 4294967290u) == 4294967290u); /* diff -5 */
    assert(elib_fl_slew_update_u32(&ctx, 4294967289u) == 4294967290u); /* diff -6: held */

    printf("PASSED\n");
}

static void test_update_u32_ladder(void) {
    printf("Test: slew update_u32 accept/reject ladder... ");
    elib_fl_slew_ctx_u32_t ctx;

    assert(elib_fl_slew_init_u32(&ctx, 5) == ELIB_FL_OK);
    assert(elib_fl_slew_set_u32(&ctx, 100) == ELIB_FL_OK);

    assert(elib_fl_slew_update_u32(&ctx, 104) == 104);            /* diff 4 */
    assert(elib_fl_slew_update_u32(&ctx, 200) == 104);            /* diff 96: held */
    assert(elib_fl_slew_update_u32(&ctx, 0) == 104);              /* diff -104: held */
    assert(elib_fl_slew_update_u32(&ctx, 99) == 99);              /* diff -5: accepted */

    /* state 99 -> 104: diff +5, exact boundary, accepted */
    assert(elib_fl_slew_update_u32(&ctx, 104) == 104);

    printf("PASSED\n");
}

static void test_set_uninitialized_u32(void) {
    printf("Test: slew uninitialized u32 behaviour... ");
    elib_fl_slew_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_slew_set_u32(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_slew_update_u32(&ctx, 100) == 0);

    printf("PASSED\n");
}

/* --- i16/u16 flavours: same ladder shapes, int32 diff (|i16-i16|
 * <= 2^16-1 always), threshold domain capped at INT16_MAX --- */

static void test_init_i16(void) {
    printf("Test: slew init_i16 rejection matrix... ");
    elib_fl_slew_ctx_i16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_slew_init_i16(NULL, 5) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_slew_init_i16(&ctx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_slew_init_i16(&ctx, -5) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_slew_init_i16(&ctx, 5) == ELIB_FL_OK);
    assert(ctx.threshold == 5);
    assert(ctx.out == 0);

    printf("PASSED\n");
}

/* ladder with state 100, threshold 5: accept (diff 4), hold (diff
 * 96), hold (diff 1104), accept at both boundary signs (diff +-5) */
static void test_update_i16_ladder(void) {
    printf("Test: slew update_i16 accept/reject ladder + full range... ");
    elib_fl_slew_ctx_i16_t ctx;

    assert(elib_fl_slew_init_i16(&ctx, 5) == ELIB_FL_OK);
    assert(elib_fl_slew_set_i16(&ctx, 100) == ELIB_FL_OK);

    assert(elib_fl_slew_update_i16(&ctx, 104) == 104);       /* diff 4 */
    assert(elib_fl_slew_update_i16(&ctx, 200) == 104);       /* diff 96: held */
    assert(elib_fl_slew_update_i16(&ctx, -1000) == 104);     /* diff 1104: held */
    assert(elib_fl_slew_update_i16(&ctx, 99) == 99);         /* diff 5: accepted */
    assert(elib_fl_slew_update_i16(&ctx, 94) == 94);         /* diff -5: accepted */

    /* full range probes, threshold == INT16_MAX */
    assert(elib_fl_slew_init_i16(&ctx, INT16_MAX) == ELIB_FL_OK);
    assert(elib_fl_slew_update_i16(&ctx, INT16_MAX) == INT16_MAX);  /* diff 32767 == threshold */
    assert(elib_fl_slew_update_i16(&ctx, INT16_MIN) == INT16_MAX);  /* |diff| 65535: held */

    /* one past the threshold from zero: rejected (|diff| = 32768) */
    assert(elib_fl_slew_set_i16(&ctx, 0) == ELIB_FL_OK);
    assert(elib_fl_slew_update_i16(&ctx, INT16_MIN) == 0);

    printf("PASSED\n");
}

static void test_set_reset_uninitialized_i16(void) {
    printf("Test: slew i16 set/reset/uninitialized... ");
    elib_fl_slew_ctx_i16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_slew_set_i16(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_slew_update_i16(&ctx, 100) == 0);
    assert(ctx.out == 0);

    elib_fl_slew_init_i16(&ctx, 5);
    elib_fl_slew_set_i16(&ctx, 42);
    assert(ctx.out == 42);
    elib_fl_slew_reset_i16(&ctx);
    assert(ctx.out == 0);
    assert(ctx.threshold == 5);          /* config kept */

    printf("PASSED\n");
}

static void test_init_u16(void) {
    printf("Test: slew init_u16 rejection matrix... ");
    elib_fl_slew_ctx_u16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_slew_init_u16(NULL, 5) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_slew_init_u16(&ctx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_slew_init_u16(&ctx, 5) == ELIB_FL_OK);

    printf("PASSED\n");
}

/* state 65535 (2^16-1): the signed diff of the extremes fits int32
 * easily; boundary accepts at diff == -5, holds at -6 */
static void test_update_u16_ladder(void) {
    printf("Test: slew update_u16 accept/reject ladder + full range... ");
    elib_fl_slew_ctx_u16_t ctx;

    assert(elib_fl_slew_init_u16(&ctx, 5) == ELIB_FL_OK);
    assert(elib_fl_slew_set_u16(&ctx, 100) == ELIB_FL_OK);

    assert(elib_fl_slew_update_u16(&ctx, 104) == 104);        /* diff 4 */
    assert(elib_fl_slew_update_u16(&ctx, 200) == 104);        /* diff 96: held */
    assert(elib_fl_slew_update_u16(&ctx, 0) == 104);          /* diff -104: held */
    assert(elib_fl_slew_update_u16(&ctx, 99) == 99);          /* diff -5: accepted */

    assert(elib_fl_slew_set_u16(&ctx, 65535) == ELIB_FL_OK);
    assert(elib_fl_slew_update_u16(&ctx, 0) == 65535);        /* diff -65535: held */
    assert(elib_fl_slew_update_u16(&ctx, 65530) == 65530);    /* diff -5: accepted */
    assert(elib_fl_slew_update_u16(&ctx, 0) == 65530);        /* diff -65530: held */

    printf("PASSED\n");
}

static void test_set_uninitialized_u16(void) {
    printf("Test: slew uninitialized u16 behaviour... ");
    elib_fl_slew_ctx_u16_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_slew_set_u16(&ctx, 1) == ELIB_FL_ERR_NOT_INITIALIZED);
    assert(elib_fl_slew_update_u16(&ctx, 100) == 0);

    printf("PASSED\n");
}

int main(void) {
    printf("=== elib-filter (slew) tests ===\n\n");

    test_init_f();
    test_update_f_ladder();
    test_set_reset_f();
    test_uninitialized_f();

    test_init_i32();
    test_update_i32_full_range();
    test_update_i32_ladder();
    test_set_uninitialized_i32();

    test_init_u32();
    test_update_u32_full_range();
    test_update_u32_ladder();
    test_set_uninitialized_u32();

    test_init_i16();
    test_update_i16_ladder();
    test_set_reset_uninitialized_i16();

    test_init_u16();
    test_update_u16_ladder();
    test_set_uninitialized_u16();

    printf("\n=== All tests passed ===\n");
    return 0;
}
