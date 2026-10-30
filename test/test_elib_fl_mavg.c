/* test_elib_fl_mavg.c - Sliding-Window Mean Filter Unit Tests (v2)
 *
 * All expectations are hand-derived exact constants:
 *  - size 3, feed 12,24,36: partial means 12,18,24 (window filling),
 *    then stays full-window: 48 -> (24+36+48)/3 = 36
 *  - i32 feed 2e9 x3: partial means 2e9, 4e9/2 = 2e9, 6e9/3 = 2e9;
 *    the running sum reaches 6e9, beyond int32 -- the v1 int32
 *    accumulator overflowed with undefined behavior (regression);
 *    next sample 0 keeps the ring honest: (2e9+2e9+0)/3 = 1333333333
 *  - u32 feed 4e9 x3 then 0: window sum 8e9 after the ring drops the
 *    first 4e9, mean 2666666666
 *  - init zeroes the user buffer (documented); reset keeps it
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "elib_fl.h"

/* --- float --- */

static void test_init_f(void) {
    printf("Test: mavg init_f zeroes buffer + rejection matrix... ");
    elib_fl_mavg_ctx_f_t ctx;
    float buf[3];
    memset(&ctx, 0, sizeof(ctx));

    memset(buf, 0xAA, sizeof(buf));        /* poison */
    assert(elib_fl_mavg_init_f(NULL, buf, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_f(&ctx, NULL, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_f(&ctx, buf, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_mavg_init_f(&ctx, buf, 3) == ELIB_FL_OK);
    assert(ctx.buf == buf);
    assert(ctx.size == 3);
    assert(ctx.sum == 0.0f);
    for (int i = 0; i < 3; i++) {
        assert(buf[i] == 0.0f);            /* init zeroes the buffer */
    }

    printf("PASSED\n");
}

static void test_fill_f(void) {
    printf("Test: mavg update_f partial->full window... ");
    elib_fl_mavg_ctx_f_t ctx;
    float buf[3];

    assert(elib_fl_mavg_init_f(&ctx, buf, 3) == ELIB_FL_OK);

    assert(elib_fl_mavg_update_f(&ctx, 12.0f) == 12.0f);   /* 12/1 */
    assert(elib_fl_mavg_ready_f(&ctx) == 0);
    assert(elib_fl_mavg_update_f(&ctx, 24.0f) == 18.0f);   /* 36/2 */
    assert(elib_fl_mavg_ready_f(&ctx) == 0);
    assert(elib_fl_mavg_update_f(&ctx, 36.0f) == 24.0f);   /* 72/3 */
    assert(elib_fl_mavg_ready_f(&ctx) == 1);

    /* full window slides: oldest (12) leaves, 48 enters */
    assert(elib_fl_mavg_update_f(&ctx, 48.0f) == 36.0f);   /* (24+36+48)/3 */
    assert(elib_fl_mavg_ready_f(&ctx) == 1);               /* stays ready */

    /* ready remains after every full-window update */
    assert(elib_fl_mavg_update_f(&ctx, 30.0f) == 38.0f);   /* (36+48+30)/3 */

    printf("PASSED\n");
}

static void test_reset_f(void) {
    printf("Test: mavg reset_f... ");
    elib_fl_mavg_ctx_f_t ctx;
    float buf[2];

    assert(elib_fl_mavg_init_f(&ctx, buf, 2) == ELIB_FL_OK);
    (void)elib_fl_mavg_update_f(&ctx, 10.0f);
    assert(elib_fl_mavg_ready_f(&ctx) == 0);

    elib_fl_mavg_reset_f(&ctx);            /* buffer NOT re-zeroed, state cleared */
    assert(ctx.sum == 0.0f);
    assert(ctx.count == 0);
    assert(ctx.bit_flags.initialized == 1);

    /* stale ring contents do not contribute: window starts fresh */
    assert(elib_fl_mavg_update_f(&ctx, 3.0f) == 3.0f);     /* count 1 again */
    assert(elib_fl_mavg_ready_f(&ctx) == 0);

    printf("PASSED\n");
}

static void test_uninitialized_f(void) {
    printf("Test: mavg uninitialized float behaviour... ");
    elib_fl_mavg_ctx_f_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_mavg_update_f(&ctx, 1.0f) == 0.0f);
    assert(elib_fl_mavg_update_f(NULL, 1.0f) == 0.0f);
    assert(elib_fl_mavg_ready_f(&ctx) == 0);
    assert(elib_fl_mavg_ready_f(NULL) == 0);
    assert(ctx.count == 0);                /* nothing moved */

    elib_fl_mavg_reset_f(NULL);
    elib_fl_mavg_reset_f(&ctx);            /* no-ops */

    printf("PASSED\n");
}

/* --- i32 --- */

static void test_init_i32(void) {
    printf("Test: mavg init_i32 zeroes buffer + rejection matrix... ");
    elib_fl_mavg_ctx_i32_t ctx;
    int32_t buf[3];
    memset(&ctx, 0, sizeof(ctx));

    memset(buf, 0xAA, sizeof(buf));
    assert(elib_fl_mavg_init_i32(NULL, buf, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_i32(&ctx, NULL, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_i32(&ctx, buf, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_mavg_init_i32(&ctx, buf, 3) == ELIB_FL_OK);
    assert(buf[0] == 0 && buf[1] == 0 && buf[2] == 0);

    printf("PASSED\n");
}

/* v1 UB regression: 2e9 x3 puts the running sum at 6e9 */
static void test_fill_i32_overflow_regression(void) {
    printf("Test: mavg update_i32 2e9x3 (v1 int32 UB regression)... ");
    elib_fl_mavg_ctx_i32_t ctx;
    int32_t buf[3];

    assert(elib_fl_mavg_init_i32(&ctx, buf, 3) == ELIB_FL_OK);

    assert(elib_fl_mavg_update_i32(&ctx, 2000000000) == 2000000000);  /* 2e9/1 - int64 safe */
    assert(elib_fl_mavg_update_i32(&ctx, 2000000000) == 2000000000);  /* 4e9/2 */
    assert(elib_fl_mavg_update_i32(&ctx, 2000000000) == 2000000000);  /* 6e9/3 */
    assert(elib_fl_mavg_ready_i32(&ctx) == 1);

    /* slide: drop oldest 2e9, add 0 -> 4e9/3 (v1: sum already UB-overflowed) */
    assert(elib_fl_mavg_update_i32(&ctx, 0) == 1333333333);           /* 4e9/3 trunc */
    assert(elib_fl_mavg_ready_i32(&ctx) == 1);

    /* negative mirror after reset */
    elib_fl_mavg_reset_i32(&ctx);
    assert(elib_fl_mavg_update_i32(&ctx, -9) == -9);                  /* -9/1 */
    assert(elib_fl_mavg_update_i32(&ctx, -4) == -6);                  /* -13/2 trunc */
    assert(elib_fl_mavg_update_i32(&ctx, -2) == -5);                  /* -15/3 */

    printf("PASSED\n");
}

static void test_uninitialized_i32(void) {
    printf("Test: mavg uninitialized i32 behaviour... ");
    elib_fl_mavg_ctx_i32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_mavg_update_i32(&ctx, 1) == 0);
    assert(elib_fl_mavg_ready_i32(&ctx) == 0);

    printf("PASSED\n");
}

/* --- u32 --- */

static void test_init_u32(void) {
    printf("Test: mavg init_u32 zeroes buffer + rejection matrix... ");
    elib_fl_mavg_ctx_u32_t ctx;
    uint32_t buf[3];
    memset(&ctx, 0, sizeof(ctx));

    memset(buf, 0xAA, sizeof(buf));
    assert(elib_fl_mavg_init_u32(NULL, buf, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_u32(&ctx, NULL, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_u32(&ctx, buf, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(ctx.bit_flags.initialized == 0);

    assert(elib_fl_mavg_init_u32(&ctx, buf, 3) == ELIB_FL_OK);
    assert(buf[0] == 0u && buf[1] == 0u && buf[2] == 0u);

    printf("PASSED\n");
}

/* v1 wraparound regression: 4e9 x3 puts the running sum at 12e9,
 * far beyond 2^32; the uint64 accumulator must be exact */
static void test_fill_u32_overflow_regression(void) {
    printf("Test: mavg update_u32 4e9x3 (v1 uint32 wrap regression)... ");
    elib_fl_mavg_ctx_u32_t ctx;
    uint32_t buf[3];

    assert(elib_fl_mavg_init_u32(&ctx, buf, 3) == ELIB_FL_OK);

    assert(elib_fl_mavg_update_u32(&ctx, 4000000000u) == 4000000000u);  /* 4e9/1 */
    assert(elib_fl_mavg_update_u32(&ctx, 4000000000u) == 4000000000u);  /* 8e9/2 */
    assert(elib_fl_mavg_update_u32(&ctx, 4000000000u) == 4000000000u);  /* 12e9/3 */
    assert(elib_fl_mavg_ready_u32(&ctx) == 1);

    /* slide: drop first 4e9, add 0 -> 8e9/3 */
    assert(elib_fl_mavg_update_u32(&ctx, 0) == 2666666666u);            /* 8e9/3 trunc */
    assert(elib_fl_mavg_update_u32(&ctx, 3) == 1333333334u);            /* (4e9+3)/3 = 1333333334.33 -> 1333333334 */

    printf("PASSED\n");
}

static void test_uninitialized_u32(void) {
    printf("Test: mavg uninitialized u32 behaviour... ");
    elib_fl_mavg_ctx_u32_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    assert(elib_fl_mavg_update_u32(&ctx, 1) == 0);
    assert(elib_fl_mavg_ready_u32(&ctx) == 0);

    printf("PASSED\n");
}

/* --- i16/u16 flavours: 32-bit accumulator, init-enforced window
 * cap 65535 (same bound argument as avg: 65535 * 32768 < 2^31 for
 * i16, 65535 * 65535 < 2^32 for u16) --- */

static void test_init_i16_u16(void) {
    printf("Test: mavg init_i16/init_u16 rejection matrices (window cap)... ");
    elib_fl_mavg_ctx_i16_t ictx;
    int16_t ibuf[3];
    memset(&ictx, 0, sizeof(ictx));
    memset(ibuf, 0xAA, sizeof(ibuf));
    elib_fl_mavg_ctx_u16_t uctx;
    uint16_t ubuf[3];
    memset(&uctx, 0, sizeof(uctx));
    memset(ubuf, 0xAA, sizeof(ubuf));

    assert(elib_fl_mavg_init_i16(NULL, ibuf, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_i16(&ictx, NULL, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_i16(&ictx, ibuf, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_i16(&ictx, ibuf, 65536u) == ELIB_FL_ERR_INVALID_PARAM);  /* cap */
    assert(ictx.bit_flags.initialized == 0);
    assert(elib_fl_mavg_init_i16(&ictx, ibuf, 65535u) == ELIB_FL_OK);                /* cap-1 accepted */

    assert(elib_fl_mavg_init_u16(NULL, ubuf, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_u16(&uctx, NULL, 3) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_u16(&uctx, ubuf, 0) == ELIB_FL_ERR_INVALID_PARAM);
    assert(elib_fl_mavg_init_u16(&uctx, ubuf, 65536u) == ELIB_FL_ERR_INVALID_PARAM);  /* cap */
    assert(elib_fl_mavg_init_u16(&uctx, ubuf, 3) == ELIB_FL_OK);
    assert(ubuf[0] == 0 && ubuf[1] == 0 && ubuf[2] == 0);    /* init zeroes buffer */

    printf("PASSED\n");
}

static void test_fill_i16(void) {
    printf("Test: mavg update_i16 partial->full + full-range slide... ");
    elib_fl_mavg_ctx_i16_t ctx;
    int16_t buf[3];

    assert(elib_fl_mavg_init_i16(&ctx, buf, 3) == ELIB_FL_OK);

    assert(elib_fl_mavg_update_i16(&ctx, 120) == 120);   /* 120/1 */
    assert(elib_fl_mavg_ready_i16(&ctx) == 0);
    assert(elib_fl_mavg_update_i16(&ctx, 240) == 180);   /* 360/2 */
    assert(elib_fl_mavg_ready_i16(&ctx) == 0);
    assert(elib_fl_mavg_update_i16(&ctx, 360) == 240);   /* 720/3 */
    assert(elib_fl_mavg_ready_i16(&ctx) == 1);

    /* slide: oldest (120) leaves, 480 enters */
    assert(elib_fl_mavg_update_i16(&ctx, 480) == 360);   /* (240+360+480)/3 */
    assert(elib_fl_mavg_ready_i16(&ctx) == 1);

    /* full-range slide: 32767 x3 fills, then INT16_MIN enters */
    elib_fl_mavg_reset_i16(&ctx);
    assert(elib_fl_mavg_update_i16(&ctx, INT16_MAX) == 32767);         /* 32767/1 */
    assert(elib_fl_mavg_update_i16(&ctx, INT16_MAX) == 32767);         /* 65534/2 */
    assert(elib_fl_mavg_update_i16(&ctx, INT16_MAX) == 32767);         /* 98301/3 */
    /* drop oldest 32767, add -32768: sum 32766 -> 10922.0 */
    assert(elib_fl_mavg_update_i16(&ctx, INT16_MIN) == 10922);

    printf("PASSED\n");
}

static void test_fill_u16(void) {
    printf("Test: mavg update_u16 partial->full + full-range slide... ");
    elib_fl_mavg_ctx_u16_t ctx;
    uint16_t buf[3];

    assert(elib_fl_mavg_init_u16(&ctx, buf, 3) == ELIB_FL_OK);

    assert(elib_fl_mavg_update_u16(&ctx, 60000u) == 60000u);   /* 60000/1 */
    assert(elib_fl_mavg_update_u16(&ctx, 60000u) == 60000u);   /* 120000/2 */
    assert(elib_fl_mavg_update_u16(&ctx, 60000u) == 60000u);   /* 180000/3 */
    assert(elib_fl_mavg_ready_u16(&ctx) == 1);

    /* slide: drop 60000, add 65535 -> (180000-60000+65535)/3 = 61845 */
    assert(elib_fl_mavg_update_u16(&ctx, 65535u) == 61845u);
    assert(elib_fl_mavg_ready_u16(&ctx) == 1);

    elib_fl_mavg_reset_u16(&ctx);
    assert(elib_fl_mavg_ready_u16(&ctx) == 0);

    printf("PASSED\n");
}

static void test_uninitialized_i16_u16(void) {
    printf("Test: mavg uninitialized i16/u16 behaviour... ");
    elib_fl_mavg_ctx_i16_t ictx;
    memset(&ictx, 0, sizeof(ictx));
    elib_fl_mavg_ctx_u16_t uctx;
    memset(&uctx, 0, sizeof(uctx));

    assert(elib_fl_mavg_update_i16(&ictx, 1) == 0);
    assert(elib_fl_mavg_ready_i16(&ictx) == 0);
    assert(elib_fl_mavg_update_u16(&uctx, 1) == 0);
    assert(elib_fl_mavg_ready_u16(&uctx) == 0);

    printf("PASSED\n");
}

int main(void) {
    printf("=== elib-filter (mavg) tests ===\n\n");

    test_init_f();
    test_fill_f();
    test_reset_f();
    test_uninitialized_f();

    test_init_i32();
    test_fill_i32_overflow_regression();
    test_uninitialized_i32();

    test_init_u32();
    test_fill_u32_overflow_regression();
    test_uninitialized_u32();

    test_init_i16_u16();
    test_fill_i16();
    test_fill_u16();
    test_uninitialized_i16_u16();

    printf("\n=== All tests passed ===\n");
    return 0;
}
