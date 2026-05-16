#include <stdint.h>
#include <am.h>
#include <klib.h>

// ---------------------------------------------------------------------------
// SDRAM mem-test — 4 KB SDRAM 访存测试
// ---------------------------------------------------------------------------

#define SDRAM_BASE 0xa0000000UL
#define TEST_SIZE  4096UL

enum {
    HALT_OK       = 0,
    HALT_U32_FAIL = 1,
    HALT_U16_FAIL = 2,
    HALT_U8_FAIL  = 3,
};

// ---------------------------------------------------------------------------
// 32-bit
// ---------------------------------------------------------------------------
static int test_u32_block(uintptr_t base, size_t size)
{
    volatile uint32_t *p = (volatile uint32_t *)base;
    size_t n = size / 4;
    size_t i;

    for (i = 0; i < n; i++)
        p[i] = (uint32_t)(base + i * 4);
    asm volatile("fence" ::: "memory");
    for (i = 0; i < n; i++) {
        uint32_t exp = (uint32_t)(base + i * 4);
        uint32_t got = p[i];
        if (got != exp) {
            int diff = (int)(exp ^ got);
            halt(diff ? diff : HALT_U32_FAIL);
            return HALT_U32_FAIL;
        }
    }
    printf("  u32 4KB PASS (%lu words)\n", (unsigned long)n);
    return HALT_OK;
}

// ---------------------------------------------------------------------------
// 16-bit
// ---------------------------------------------------------------------------
static int test_u16_block(uintptr_t base, size_t size)
{
    volatile uint16_t *p = (volatile uint16_t *)base;
    size_t n = size / 2;
    size_t i;

    for (i = 0; i < n; i++)
        p[i] = (uint16_t)((base + i * 2) & 0xffff);
    asm volatile("fence" ::: "memory");
    for (i = 0; i < n; i++) {
        uint16_t exp = (uint16_t)((base + i * 2) & 0xffff);
        uint16_t got = p[i];
        if (got != exp) {
            int diff = (int)(exp ^ got);
            halt(diff ? diff : HALT_U16_FAIL);
            return HALT_U16_FAIL;
        }
    }
    printf("  u16 4KB PASS (%lu words)\n", (unsigned long)n);
    return HALT_OK;
}

// ---------------------------------------------------------------------------
// 8-bit
// ---------------------------------------------------------------------------
static int test_u8_block(uintptr_t base, size_t size)
{
    volatile uint8_t *p = (volatile uint8_t *)base;
    size_t i;

    for (i = 0; i < size; i++)
        p[i] = (uint8_t)((base + i) & 0xff);
    asm volatile("fence" ::: "memory");
    for (i = 0; i < size; i++) {
        uint8_t exp = (uint8_t)((base + i) & 0xff);
        uint8_t got = p[i];
        if (got != exp) {
            int diff = (int)(exp ^ got);
            halt(diff ? diff : HALT_U8_FAIL);
            return HALT_U8_FAIL;
        }
    }
    printf("  u8  4KB PASS (%lu bytes)\n", (unsigned long)size);
    return HALT_OK;
}

// ---------------------------------------------------------------------------
int main(const char *mainargs)
{
    uintptr_t base = SDRAM_BASE;
    int ret;

    (void)mainargs;

    printf("SDRAM mem-test: 0x%08lx - 0x%08lx (%lu KB)\n",
           (unsigned long)base,
           (unsigned long)(base + TEST_SIZE - 1),
           (unsigned long)(TEST_SIZE / 1024));
    printf("Total length: %lu bytes\n", (unsigned long)TEST_SIZE);

    ret = test_u32_block(base, TEST_SIZE);
    if (ret != HALT_OK) halt(ret);

    ret = test_u16_block(base, TEST_SIZE);
    if (ret != HALT_OK) halt(ret);

    ret = test_u8_block(base, TEST_SIZE);
    if (ret != HALT_OK) halt(ret);

    printf("SDRAM mem-test: ALL TESTS PASSED\n");
    halt(HALT_OK);
    return 0;
}