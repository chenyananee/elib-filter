/* test_elib_fl_median.c - Batch Median Filter Unit Tests (v2)
 *
 * All expectations are hand-derived:
 *  - odd window size 3, batch [5,1,3]: sorted [1,3,5], median
 *    buf[size/2] = buf[1] = 3
 *  - even window size 4, batch [7,100,5,42]: sorted [5,7,42,100],
 *    median buf[2] = 42 (larger middle element, by definition)
 *  - full-range int32 batch [INT32_MAX, INT32_MIN, 0]: sorted
 *    [INT32_MIN, 0, INT32_MAX], median 0
 *  - the user buffer is sorted IN PLACE: its order after each output
 *    is asserted (documented side effect, not an accident)
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "elib_fl.h"

/* --- float --- */

static void test_init_f(void) {
    printf("Test: median init_f rejection matrix... ");
    elib_fl_median_ctx_f_t ctx;
    float buf[4];
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_median_init_f(NULL, buf, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_f(&ctx, NULL, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_f(&ctx, buf, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_median_init_f(&ctx, buf, 3) == ELIB_FL_OK);
    assert(ctx.buf == buf);
    assert(ctx.size == 3);
    assert(ctx.count == 0);
    assert(ctx.bit_flags.initialized == 1);

    printf("PASSED\n");
}

/* size 3, batches cycle: [5,1,3] -> median 3; next sample restarts */
static void test_batch_f(void) {
    printf("Test: median update_f odd window... ");
    elib_fl_median_ctx_f_t ctx;
    float buf[3];

    assert(elib_fl_median_init_f(&ctx, buf, 3) == ELIB_FL_OK);

    assert(elib_fl_median_update_f(&ctx, 5.0f) == 0.0f);
    assert(elib_fl_median_ready_f(&ctx) == 0);
    assert(elib_fl_median_update_f(&ctx, 1.0f) == 0.0f);
    assert(elib_fl_median_ready_f(&ctx) == 0);
    assert(elib_fl_median_update_f(&ctx, 3.0f) == 3.0f);
    assert(elib_fl_median_ready_f(&ctx) == 1);

    /* in-place sort side effect, documented */
    assert(buf[0] == 1.0f && buf[1] == 3.0f && buf[2] == 5.0f);

    /* window restarted: ready flips back, then a new batch */
    assert(elib_fl_median_ready_f(&ctx) == 1);
    assert(elib_fl_median_update_f(&ctx, 9.0f) == 0.0f);
    assert(elib_fl_median_ready_f(&ctx) == 0);
    assert(elib_fl_median_update_f(&ctx, 9.0f) == 0.0f);
    assert(elib_fl_median_update_f(&ctx, 9.0f) == 9.0f);
    assert(elib_fl_median_ready_f(&ctx) == 1);

    printf("PASSED\n");
}

/* even window: buf[size/2] = buf[2] of the sorted buffer */
static void test_batch_even_f(void) {
    printf("Test: median update_f even window (larger middle)... ");
    elib_fl_median_ctx_f_t ctx;
    float buf[4];

    assert(elib_fl_median_init_f(&ctx, buf, 4) == ELIB_FL_OK);

    assert(elib_fl_median_update_f(&ctx, 7.0f) == 0.0f);
    assert(elib_fl_median_update_f(&ctx, 100.0f) == 0.0f);
    assert(elib_fl_median_update_f(&ctx, 5.0f) == 0.0f);
    assert(elib_fl_median_update_f(&ctx, 42.0f) == 42.0f);   /* sorted [5,7,42,100] */
    assert(buf[0] == 5.0f && buf[1] == 7.0f && buf[2] == 42.0f && buf[3] == 100.0f);

    printf("PASSED\n");
}

static void test_reset_f(void) {
    printf("Test: median reset_f... ");
    elib_fl_median_ctx_f_t ctx;
    float buf[3];

    assert(elib_fl_median_init_f(&ctx, buf, 3) == ELIB_FL_OK);
    (void)elib_fl_median_update_f(&ctx, 5.0f);
    (void)elib_fl_median_update_f(&ctx, 1.0f);
    assert(elib_fl_median_ready_f(&ctx) == 0);

    elib_fl_median_reset_f(&ctx);       /* discard partial window */
    assert(ctx.count == 0);
    assert(elib_fl_median_ready_f(&ctx) == 0);
    assert(ctx.bit_flags.initialized == 1);

    /* a full batch right after reset works */
    (void)elib_fl_median_update_f(&ctx, 2.0f);
    (void)elib_fl_median_update_f(&ctx, 8.0f);
    (void)elib_fl_median_update_f(&ctx, 4.0f);
    assert(elib_fl_median_update_f(&ctx, -1.0f) == 0.0f);    /* count==1 again */
    (void)elib_fl_median_update_f(&ctx, 2.0f);
    (void)elib_fl_median_update_f(&ctx, 8.0f);
    assert(elib_fl_median_update_f(&ctx, 4.0f) == 4.0f);     /* [2,4,8] */

    printf("PASSED\n");
}

static void test_uninitialized_f(void) {
    printf("Test: median uninitialized float behaviour... ");
    elib_fl_median_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_median_update_f(&ctx, 1.0f) == 0.0f);
    assert(elib_fl_median_update_f(NULL, 1.0f) == 0.0f);
    assert(elib_fl_median_ready_f(&ctx) == 0);
    assert(elib_fl_median_ready_f(NULL) == 0);

    elib_fl_median_reset_f(NULL);
    elib_fl_median_reset_f(&ctx);       /* no-ops */

    printf("PASSED\n");
}

/* --- i32 --- */

static void test_init_i32(void) {
    printf("Test: median init_i32 rejection matrix... ");
    elib_fl_median_ctx_i32_t ctx;
    int32_t buf[3];
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_median_init_i32(NULL, buf, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_i32(&ctx, NULL, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_i32(&ctx, buf, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_median_init_i32(&ctx, buf, 3) == ELIB_FL_OK);
    assert(ctx.size == 3);

    printf("PASSED\n");
}

/* full-range batch: sorting must handle INT32_MIN/INT32_MAX */
static void test_batch_i32(void) {
    printf("Test: median update_i32 full-range batch... ");
    elib_fl_median_ctx_i32_t ctx;
    int32_t buf[3];

    assert(elib_fl_median_init_i32(&ctx, buf, 3) == ELIB_FL_OK);

    (void)elib_fl_median_update_i32(&ctx, INT32_MAX);
    (void)elib_fl_median_update_i32(&ctx, INT32_MIN);
    assert(elib_fl_median_update_i32(&ctx, 0) == 0);     /* sorted [MIN,0,MAX] */
    assert(buf[0] == INT32_MIN && buf[1] == 0 && buf[2] == INT32_MAX);

    /* duplicates are fine */
    (void)elib_fl_median_update_i32(&ctx, 7);
    (void)elib_fl_median_update_i32(&ctx, 7);
    assert(elib_fl_median_update_i32(&ctx, 7) == 7);

    printf("PASSED\n");
}

static void test_reset_ready_i32(void) {
    printf("Test: median i32 ready/reset cycle... ");
    elib_fl_median_ctx_i32_t ctx;
    int32_t buf[2];

    assert(elib_fl_median_init_i32(&ctx, buf, 2) == ELIB_FL_OK);
    (void)elib_fl_median_update_i32(&ctx, 4);
    (void)elib_fl_median_update_i32(&ctx, 1);            /* sorted [1,4] -> buf[1] */
    assert(elib_fl_median_update_i32(&ctx, 9) == 0);     /* window restarted */
    (void)elib_fl_median_update_i32(&ctx, 0);
    assert(elib_fl_median_update_i32(&ctx, 5) == 5);     /* sorted [0,5] */

    elib_fl_median_reset_i32(&ctx);
    assert(elib_fl_median_ready_i32(&ctx) == 0);

    printf("PASSED\n");
}

static void test_uninitialized_i32(void) {
    printf("Test: median uninitialized i32 behaviour... ");
    elib_fl_median_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_median_update_i32(&ctx, 1) == 0);
    assert(elib_fl_median_ready_i32(&ctx) == 0);

    printf("PASSED\n");
}

/* --- u32 --- */

static void test_init_u32(void) {
    printf("Test: median init_u32 rejection matrix... ");
    elib_fl_median_ctx_u32_t ctx;
    uint32_t buf[3];
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_median_init_u32(NULL, buf, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_u32(&ctx, NULL, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_u32(&ctx, buf, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_median_init_u32(&ctx, buf, 3) == ELIB_FL_OK);

    printf("PASSED\n");
}

static void test_batch_u32(void) {
    printf("Test: median update_u32 full-range batch... ");
    elib_fl_median_ctx_u32_t ctx;
    uint32_t buf[3];

    assert(elib_fl_median_init_u32(&ctx, buf, 3) == ELIB_FL_OK);

    (void)elib_fl_median_update_u32(&ctx, 4294967295u);
    (void)elib_fl_median_update_u32(&ctx, 1);
    assert(elib_fl_median_update_u32(&ctx, 5) == 5);     /* sorted [1,5,MAX] */
    assert(buf[0] == 1u && buf[1] == 5u && buf[2] == 4294967295u);

    printf("PASSED\n");
}

static void test_uninitialized_u32(void) {
    printf("Test: median uninitialized u32 behaviour... ");
    elib_fl_median_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_median_update_u32(&ctx, 1) == 0);
    assert(elib_fl_median_ready_u32(&ctx) == 0);

    printf("PASSED\n");
}

/* --- i16/u16 flavours: same batch rhythm; sorting handles the
 * full-range extremes INT16_MIN/INT16_MAX (and 65535 for u16) --- */

static void test_init_i16_u16(void) {
    printf("Test: median init_i16/init_u16 rejection matrices... ");
    elib_fl_median_ctx_i16_t ictx;
    int16_t ibuf[3];
    elib_fl_median_ctx_u16_t uctx;
    uint16_t ubuf[3];
    memset(&ictx, 0, sizeof(ictx));
    memset(&uctx, 0, sizeof(uctx));

    assert(elib_fl_median_init_i16(NULL, ibuf, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_i16(&ictx, NULL, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_i16(&ictx, ibuf, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ictx.bit_flags.initialized == 0);
    assert(elib_fl_median_init_i16(&ictx, ibuf, 3) == ELIB_FL_OK);

    assert(elib_fl_median_init_u16(NULL, ubuf, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_u16(&uctx, NULL, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_u16(&uctx, ubuf, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_median_init_u16(&uctx, ubuf, 3) == ELIB_FL_OK);

    printf("PASSED\n");
}

static void test_batch_i16(void) {
    printf("Test: median update_i16 batch + full-range batch... ");
    elib_fl_median_ctx_i16_t ctx;
    int16_t buf[3];

    assert(elib_fl_median_init_i16(&ctx, buf, 3) == ELIB_FL_OK);

    /* plain batch [5,1,3] -> sorted [1,3,5] -> median buf[1] = 3 */
    (void)elib_fl_median_update_i16(&ctx, 5);
    (void)elib_fl_median_update_i16(&ctx, 1);
    assert(elib_fl_median_update_i16(&ctx, 3) == 3);
    assert(elib_fl_median_ready_i16(&ctx) == 1);
    assert(buf[0] == 1 && buf[1] == 3 && buf[2] == 5);   /* sorted in place */

    /* full-range extremes sort correctly */
    (void)elib_fl_median_update_i16(&ctx, INT16_MAX);
    (void)elib_fl_median_update_i16(&ctx, INT16_MIN);
    assert(elib_fl_median_update_i16(&ctx, 0) == 0);    /* [MIN, 0, MAX] */
    assert(buf[0] == INT16_MIN && buf[1] == 0 && buf[2] == INT16_MAX);

    /* window restarted: warmup sentinel + ready flip */
    assert(elib_fl_median_update_i16(&ctx, 9) == 0);
    assert(elib_fl_median_ready_i16(&ctx) == 0);
    (void)elib_fl_median_update_i16(&ctx, 9);
    (void)elib_fl_median_update_i16(&ctx, 9);
    assert(elib_fl_median_ready_i16(&ctx) == 1);

    elib_fl_median_reset_i16(&ctx);
    assert(elib_fl_median_ready_i16(&ctx) == 0);

    printf("PASSED\n");
}

static void test_batch_u16(void) {
    printf("Test: median update_u16 batch + full-range batch... ");
    elib_fl_median_ctx_u16_t ctx;
    uint16_t buf[3];

    assert(elib_fl_median_init_u16(&ctx, buf, 3) == ELIB_FL_OK);

    (void)elib_fl_median_update_u16(&ctx, 5);
    (void)elib_fl_median_update_u16(&ctx, 65535u);
    assert(elib_fl_median_update_u16(&ctx, 1) == 5);    /* sorted [1, 5, 65535] */
    assert(buf[0] == 1 && buf[1] == 5 && buf[2] == 65535);

    /* even window: larger middle element */
    elib_fl_median_ctx_u16_t ectx;
    uint16_t ebuf[2];
    assert(elib_fl_median_init_u16(&ectx, ebuf, 2) == ELIB_FL_OK);
    (void)elib_fl_median_update_u16(&ectx, 4);
    assert(elib_fl_median_update_u16(&ectx, 1) == 4);   /* sorted [1,4] -> buf[1] */

    assert(elib_fl_median_update_u16(&ectx, 9) == 0);   /* warmup sentinel */
    assert(elib_fl_median_ready_u16(&ectx) == 0);

    printf("PASSED\n");
}

static void test_uninitialized_i16_u16(void) {
    printf("Test: median uninitialized i16/u16 behaviour... ");
    elib_fl_median_ctx_i16_t ictx;
    elib_fl_median_ctx_u16_t uctx;
    memset(&ictx, 0, sizeof(ictx));
    memset(&uctx, 0, sizeof(uctx));

    assert(elib_fl_median_update_i16(&ictx, 1) == 0);
    assert(elib_fl_median_ready_i16(&ictx) == 0);
    assert(elib_fl_median_update_u16(&uctx, 1) == 0);
    assert(elib_fl_median_ready_u16(&uctx) == 0);

    printf("PASSED\n");
}

int main(void) {
    printf("=== elib-filter (median) tests ===\n\n");

    test_init_f();
    test_batch_f();
    test_batch_even_f();
    test_reset_f();
    test_uninitialized_f();

    test_init_i32();
    test_batch_i32();
    test_reset_ready_i32();
    test_uninitialized_i32();

    test_init_u32();
    test_batch_u32();
    test_uninitialized_u32();

    test_init_i16_u16();
    test_batch_i16();
    test_batch_u16();
    test_uninitialized_i16_u16();

    printf("\n=== All tests passed ===\n");
    return 0;
}
