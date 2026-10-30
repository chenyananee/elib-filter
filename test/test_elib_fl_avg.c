/* test_elib_fl_avg.c - Batch Average Filter Unit Tests (v2)
 *
 * All expectations are hand-derived exact constants:
 *  - float batch [1,3] size 2 -> 2.0f (dyadic, exact)
 *  - i32 batch [2e9,2e9] size 2 -> 2e9: sum = 4e9 fits in int64
 *    but NOT in int32 -- the v1 accumulator overflowed with
 *    undefined behavior on exactly this input (regression)
 *  - i32 truncation: [1,2] -> 3/2 = 1, [-1,-2] -> -3/2 = -1
 *    (toward zero, NOT floor)
 *  - u32 batch [3e9,3e9] size 2 -> 3e9: sum = 6e9 exceeds 2^32;
 *    the v1 uint32 accumulator wrapped to 1705032704 and returned
 *    852516352 (regression)
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "elib_fl.h"

/* --- float --- */

static void test_init_f(void) {
    printf("Test: avg init_f rejection matrix... ");
    elib_fl_avg_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_avg_init_f(NULL, 4) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_avg_init_f(&ctx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_avg_init_f(&ctx, 4) == ELIB_FL_OK);
    assert(ctx.size == 4);
    assert(ctx.count == 0);
    assert(ctx.sum == 0.0f);

    printf("PASSED\n");
}

static void test_batch_f(void) {
    printf("Test: avg update_f batch mean... ");
    elib_fl_avg_ctx_f_t ctx;

    assert(elib_fl_avg_init_f(&ctx, 2) == ELIB_FL_OK);

    assert(elib_fl_avg_update_f(&ctx, 1.0f) == 0.0f);    /* warmup sentinel */
    assert(elib_fl_avg_ready_f(&ctx) == 0);
    assert(elib_fl_avg_update_f(&ctx, 3.0f) == 2.0f);    /* (1+3)/2 */
    assert(elib_fl_avg_ready_f(&ctx) == 1);

    /* window restarted */
    assert(elib_fl_avg_update_f(&ctx, 5.0f) == 0.0f);
    assert(elib_fl_avg_ready_f(&ctx) == 0);
    assert(elib_fl_avg_update_f(&ctx, 7.0f) == 6.0f);    /* (5+7)/2 */
    assert(elib_fl_avg_ready_f(&ctx) == 1);

    /* negative values */
    assert(elib_fl_avg_update_f(&ctx, -5.0f) == 0.0f);
    assert(elib_fl_avg_update_f(&ctx, 5.0f) == 0.0f);    /* windup: sum -5+5 = 0 */

    printf("PASSED\n");
}

static void test_reset_f(void) {
    printf("Test: avg reset_f discards partial window... ");
    elib_fl_avg_ctx_f_t ctx;

    assert(elib_fl_avg_init_f(&ctx, 2) == ELIB_FL_OK);
    (void)elib_fl_avg_update_f(&ctx, 100.0f);
    elib_fl_avg_reset_f(&ctx);
    assert(ctx.sum == 0.0f);
    assert(ctx.count == 0);
    assert(elib_fl_avg_ready_f(&ctx) == 0);
    assert(ctx.size == 2);                                /* config kept */

    /* stale sample must not leak into the next batch */
    (void)elib_fl_avg_update_f(&ctx, 2.0f);
    (void)elib_fl_avg_update_f(&ctx, 4.0f);
    assert(ctx.count == 0 && ctx.sum == 0.0f);            /* batch closed */

    printf("PASSED\n");
}

static void test_uninitialized_f(void) {
    printf("Test: avg uninitialized float behaviour... ");
    elib_fl_avg_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_avg_update_f(&ctx, 1.0f) == 0.0f);
    assert(elib_fl_avg_update_f(NULL, 1.0f) == 0.0f);
    assert(elib_fl_avg_ready_f(&ctx) == 0);
    assert(elib_fl_avg_ready_f(NULL) == 0);

    elib_fl_avg_reset_f(NULL);
    elib_fl_avg_reset_f(&ctx);                            /* no-ops */

    printf("PASSED\n");
}

/* --- i32 --- */

static void test_init_i32(void) {
    printf("Test: avg init_i32 rejection matrix... ");
    elib_fl_avg_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_avg_init_i32(NULL, 4) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_avg_init_i32(&ctx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_avg_init_i32(&ctx, 4) == ELIB_FL_OK);

    printf("PASSED\n");
}

/* v1 UB regression: two 2000000000 samples sum to 4e9, far beyond
 * INT32_MAX; the 64-bit accumulator handles it exactly */
static void test_batch_i32_overflow_regression(void) {
    printf("Test: avg update_i32 2e9+2e9 (v1 int32 UB regression)... ");
    elib_fl_avg_ctx_i32_t ctx;

    assert(elib_fl_avg_init_i32(&ctx, 2) == ELIB_FL_OK);

    assert(elib_fl_avg_update_i32(&ctx, 2000000000) == 0);            /* sentinel */
    assert(elib_fl_avg_update_i32(&ctx, 2000000000) == 2000000000);   /* 4e9/2 */

    /* negative mirror */
    assert(elib_fl_avg_update_i32(&ctx, -2000000000) == 0);
    assert(elib_fl_avg_update_i32(&ctx, -2000000000) == -2000000000); /* -4e9/2 */

    printf("PASSED\n");
}

static void test_trunc_i32(void) {
    printf("Test: avg update_i32 truncation toward zero... ");
    elib_fl_avg_ctx_i32_t ctx;

    assert(elib_fl_avg_init_i32(&ctx, 2) == ELIB_FL_OK);

    (void)elib_fl_avg_update_i32(&ctx, 1);
    assert(elib_fl_avg_update_i32(&ctx, 2) == 1);        /* 3/2 trunc -> 1 */

    assert(elib_fl_avg_update_i32(&ctx, -1) == 0);       /* window restarted */
    assert(elib_fl_avg_update_i32(&ctx, -2) == -1);      /* -3/2 -> -1, NOT floor(-2) */

    /* mixed signs */
    assert(elib_fl_avg_update_i32(&ctx, -3) == 0);
    assert(elib_fl_avg_update_i32(&ctx, 0) == -1);       /* -3/2 -> -1 */

    printf("PASSED\n");
}

static void test_uninitialized_i32(void) {
    printf("Test: avg uninitialized i32 behaviour... ");
    elib_fl_avg_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_avg_update_i32(&ctx, 1) == 0);
    assert(elib_fl_avg_ready_i32(&ctx) == 0);

    printf("PASSED\n");
}

/* --- u32 --- */

static void test_init_u32(void) {
    printf("Test: avg init_u32 rejection matrix... ");
    elib_fl_avg_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_avg_init_u32(NULL, 4) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_avg_init_u32(&ctx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_avg_init_u32(&ctx, 4) == ELIB_FL_OK);

    printf("PASSED\n");
}

/* v1 wraparound regression: 3e9+3e9 = 6e9 > 2^32; v1's uint32 sum
 * wrapped to 1705032704 and returned 852516352. v2 must return 3e9 */
static void test_batch_u32_overflow_regression(void) {
    printf("Test: avg update_u32 3e9+3e9 (v1 uint32 wrap regression)... ");
    elib_fl_avg_ctx_u32_t ctx;

    assert(elib_fl_avg_init_u32(&ctx, 2) == ELIB_FL_OK);

    assert(elib_fl_avg_update_u32(&ctx, 3000000000u) == 0);              /* sentinel */
    assert(elib_fl_avg_update_u32(&ctx, 3000000000u) == 3000000000u);    /* 6e9/2 */

    /* full-range batch */
    assert(elib_fl_avg_update_u32(&ctx, 4294967295u) == 0);
    assert(elib_fl_avg_update_u32(&ctx, 4294967295u) == 4294967295u);    /* 2*(2^32-1)/2, uint64 exact */

    printf("PASSED\n");
}

static void test_uninitialized_u32(void) {
    printf("Test: avg uninitialized u32 behaviour... ");
    elib_fl_avg_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_avg_update_u32(&ctx, 1) == 0);
    assert(elib_fl_avg_ready_u32(&ctx) == 0);

    printf("PASSED\n");
}

/* --- i16/u16 flavours: 32-bit accumulator with the init-enforced
 * window cap. The cap argument (why 65535): with size <= 65535 the
 * worst case sum |i16| = 65535 * 32768 = 2147420416 < 2^31 (i16)
 * and 65535 * 65535 = 4294901775 < 2^32 (u16); at size 65536 the
 * worst cases hit exactly 2^31 / overflow into 2^32 territory --
 * so anything larger is rejected at init, never trusted to luck. */

static void test_init_i16_u16(void) {
    printf("Test: avg init_i16/init_u16 rejection matrices (window cap)... ");
    elib_fl_avg_ctx_i16_t ictx;
    memset(&ictx, 0, sizeof(ictx));
    elib_fl_avg_ctx_u16_t uctx;
    memset(&uctx, 0, sizeof(uctx));

    assert(elib_fl_avg_init_i16(NULL, 4) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_avg_init_i16(&ictx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_avg_init_i16(&ictx, 65536u) == ELIB_FL_ERR_INVALID_PARAM);  /* cap */
    assert(elib_fl_avg_init_i16(&ictx, 4294967295u) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ictx.bit_flags.initialized == 0);
    assert(elib_fl_avg_init_i16(&ictx, 65535u) == ELIB_FL_OK);                 /* cap-1 accepted */

    assert(elib_fl_avg_init_u16(NULL, 4) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_avg_init_u16(&uctx, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_avg_init_u16(&uctx, 65536u) == ELIB_FL_ERR_INVALID_PARAM);  /* cap */
    assert(uctx.bit_flags.initialized == 0);
    assert(elib_fl_avg_init_u16(&uctx, 65535u) == ELIB_FL_OK);

    printf("PASSED\n");
}

static void test_batch_i16(void) {
    printf("Test: avg update_i16 batch means... ");
    elib_fl_avg_ctx_i16_t ctx;

    assert(elib_fl_avg_init_i16(&ctx, 2) == ELIB_FL_OK);

    assert(elib_fl_avg_update_i16(&ctx, 1000) == 0);     /* warmup sentinel */
    assert(elib_fl_avg_ready_i16(&ctx) == 0);
    assert(elib_fl_avg_update_i16(&ctx, -500) == 250);   /* (1000-500)/2 */
    assert(elib_fl_avg_ready_i16(&ctx) == 1);

    assert(elib_fl_avg_update_i16(&ctx, 1) == 0);        /* window restarted */
    assert(elib_fl_avg_update_i16(&ctx, 2) == 1);        /* 3/2 trunc */

    assert(elib_fl_avg_update_i16(&ctx, -1) == 0);
    assert(elib_fl_avg_update_i16(&ctx, -2) == -1);      /* -3/2 -> -1, NOT floor */

    /* 32-bit accumulator regression: INT16_MAX paired with itself
     * pushes the sum to 65534 (would wrap if the sum were 16-bit) */
    assert(elib_fl_avg_update_i16(&ctx, INT16_MAX) == 0);
    assert(elib_fl_avg_update_i16(&ctx, INT16_MAX) == 32767);

    printf("PASSED\n");
}

static void test_batch_u16(void) {
    printf("Test: avg update_u16 batch means... ");
    elib_fl_avg_ctx_u16_t ctx;

    assert(elib_fl_avg_init_u16(&ctx, 2) == ELIB_FL_OK);

    assert(elib_fl_avg_update_u16(&ctx, 49152u) == 0);              /* sentinel */
    assert(elib_fl_avg_update_u16(&ctx, 49152u) == 49152u);         /* 98304/2 */

    /* full-range pair: sum 131070 stays exact in uint32 */
    assert(elib_fl_avg_update_u16(&ctx, 65535u) == 0);
    assert(elib_fl_avg_update_u16(&ctx, 65535u) == 65535u);

    assert(elib_fl_avg_ready_u16(&ctx) == 1);
    elib_fl_avg_reset_u16(&ctx);
    assert(elib_fl_avg_ready_u16(&ctx) == 0);

    printf("PASSED\n");
}

static void test_uninitialized_i16_u16(void) {
    printf("Test: avg uninitialized i16/u16 behaviour... ");
    elib_fl_avg_ctx_i16_t ictx;
    memset(&ictx, 0, sizeof(ictx));
    elib_fl_avg_ctx_u16_t uctx;
    memset(&uctx, 0, sizeof(uctx));

    assert(elib_fl_avg_update_i16(&ictx, 1) == 0);
    assert(elib_fl_avg_ready_i16(&ictx) == 0);
    assert(elib_fl_avg_update_u16(&uctx, 1) == 0);
    assert(elib_fl_avg_ready_u16(&uctx) == 0);

    printf("PASSED\n");
}

int main(void) {
    printf("=== elib-filter (avg) tests ===\n\n");

    test_init_f();
    test_batch_f();
    test_reset_f();
    test_uninitialized_f();

    test_init_i32();
    test_batch_i32_overflow_regression();
    test_trunc_i32();
    test_uninitialized_i32();

    test_init_u32();
    test_batch_u32_overflow_regression();
    test_uninitialized_u32();

    test_init_i16_u16();
    test_batch_i16();
    test_batch_u16();
    test_uninitialized_i16_u16();

    printf("\n=== All tests passed ===\n");
    return 0;
}
