/*
 * SDRAM 完整访存测试 (mem-test)
 *
 * 测试对象: MT48LC16M16A2 SDRAM 颗粒
 *   容量: 4 bank x 8192 row x 512 col x 16 bit = 256 Mbit = 32 MB
 *   地址: 0xa0000000 ~ 0xa1ffffff (ysyxSoC 中映射)
 *   耗时: ~1 小时 (仿真环境)
 *
 * 测试方法 (课程规定):
 *   1. 8/16/32/64-bit 多宽度, data = addr & width_mask
 *   2. 两趟法: 全写 → 全读校验
 *   3. 1 MB 分块, 块间 printf 进度
 *   4. 首/中/尾块额外执行 allbits / checkerboard / ones_zeros
 */
#include <stdint.h>
#include <klib.h>

#define SDRAM_BASE       0xa0000000UL
#define SDRAM_SIZE       (32UL * 1024 * 1024)
#define BLOCK_SIZE       (1UL * 1024 * 1024)
#define BLOCK_COUNT      (SDRAM_SIZE / BLOCK_SIZE)
#define WORDS_PER_BLOCK  (BLOCK_SIZE / 4)

#define DIAG_BLOCK(b) ((b) == 0 || (b) == BLOCK_COUNT / 2 || (b) == (int)BLOCK_COUNT - 1)

enum {
    HALT_OK       = 0,
    HALT_U8       = 1,
    HALT_U16      = 2,
    HALT_U32      = 3,
    HALT_U64      = 4,
    HALT_ALLBITS  = 5,
    HALT_CHECKER  = 6,
    HALT_ONES     = 7,
};

static void test_u8_block(uintptr_t base, size_t size)
{
    volatile uint8_t *p = (volatile uint8_t *)base;
    size_t i;

    for (i = 0; i < size; i++)
        p[i] = (uint8_t)((base + i) & 0xff);
    for (i = 0; i < size; i++) {
        uint8_t exp = (uint8_t)((base + i) & 0xff);
        if (p[i] != exp) {
            printf("FAIL u8 @0x%08lx: exp %02x got %02x\n",
                   (unsigned long)(base + i), exp, p[i]);
            halt(HALT_U8);
        }
    }
}

static void test_u16_block(uintptr_t base, size_t size)
{
    volatile uint16_t *p = (volatile uint16_t *)base;
    size_t n = size / 2;
    size_t i;

    for (i = 0; i < n; i++)
        p[i] = (uint16_t)((base + i * 2) & 0xffff);
    for (i = 0; i < n; i++) {
        uint16_t exp = (uint16_t)((base + i * 2) & 0xffff);
        if (p[i] != exp) {
            printf("FAIL u16 @0x%08lx: exp %04x got %04x\n",
                   (unsigned long)(base + i * 2), exp, p[i]);
            halt(HALT_U16);
        }
    }
}

static void test_u32_block(uintptr_t base, size_t size)
{
    volatile uint32_t *p = (volatile uint32_t *)base;
    size_t n = size / 4;
    size_t i;

    for (i = 0; i < n; i++)
        p[i] = (uint32_t)(base + i * 4);
    for (i = 0; i < n; i++) {
        uint32_t exp = (uint32_t)(base + i * 4);
        if (p[i] != exp) {
            printf("FAIL u32 @0x%08lx: exp %08lx got %08lx\n",
                   (unsigned long)(base + i * 4),
                   (unsigned long)exp, (unsigned long)p[i]);
            halt(HALT_U32);
        }
    }
}

static void test_u64_block(uintptr_t base, size_t size)
{
    volatile uint64_t *p = (volatile uint64_t *)base;
    size_t n = size / 8;
    size_t i;

    for (i = 0; i < n; i++)
        p[i] = (uint64_t)(base + i * 8);
    for (i = 0; i < n; i++) {
        uint64_t exp = (uint64_t)(base + i * 8);
        if (p[i] != exp) {
            printf("FAIL u64 @0x%08lx: exp %08lx_%08lx got %08lx_%08lx\n",
                   (unsigned long)(base + i * 8),
                   (unsigned long)((uint32_t)(exp >> 32)),
                   (unsigned long)((uint32_t)exp),
                   (unsigned long)((uint32_t)(p[i] >> 32)),
                   (unsigned long)((uint32_t)p[i]));
            halt(HALT_U64);
        }
    }
}

static void test_u32_allbits(volatile uint32_t *p)
{
    uint32_t val = 1;
    int i;

    for (i = 0; i < 32; i++) { p[i] = val; val <<= 1; }
    p[32] = 0x55555555;
    p[33] = 0xAAAAAAAA;
    p[34] = 0xFFFFFFFF;
    p[35] = 0x00000000;

    val = 1;
    for (i = 0; i < 32; i++) {
        if (p[i] != val) {
            printf("FAIL allbits[%d]: exp %08lx got %08lx\n",
                   i, (unsigned long)val, (unsigned long)p[i]);
            halt(HALT_ALLBITS);
        }
        val <<= 1;
    }
    if (p[32] != 0x55555555) { printf("FAIL allbits[32]\n"); halt(HALT_ALLBITS); }
    if (p[33] != 0xAAAAAAAA) { printf("FAIL allbits[33]\n"); halt(HALT_ALLBITS); }
    if (p[34] != 0xFFFFFFFF) { printf("FAIL allbits[34]\n"); halt(HALT_ALLBITS); }
    if (p[35] != 0x00000000) { printf("FAIL allbits[35]\n"); halt(HALT_ALLBITS); }
}

static void test_u32_checkerboard(volatile uint32_t *p, int n)
{
    int i;

    for (i = 0; i < n; i++)
        p[i] = (i & 1) ? 0xAAAAAAAA : 0x55555555;
    for (i = 0; i < n; i++) {
        uint32_t exp = (uint32_t)((i & 1) ? 0xAAAAAAAA : 0x55555555);
        if (p[i] != exp) {
            printf("FAIL checker[%d]: exp %08lx got %08lx\n",
                   i, (unsigned long)exp, (unsigned long)p[i]);
            halt(HALT_CHECKER);
        }
    }
}

static void test_u32_ones_zeros(volatile uint32_t *p, int n)
{
    int i;

    for (i = 0; i < n; i++) p[i] = 0xFFFFFFFF;
    for (i = 0; i < n; i++) {
        if (p[i] != 0xFFFFFFFF) {
            printf("FAIL ones[%d]: got %08lx\n", i, (unsigned long)p[i]);
            halt(HALT_ONES);
        }
    }

    for (i = 0; i < n; i++) p[i] = 0x00000000;
    for (i = 0; i < n; i++) {
        if (p[i] != 0x00000000) {
            printf("FAIL zeros[%d]: got %08lx\n", i, (unsigned long)p[i]);
            halt(HALT_ONES);
        }
    }
}

int main(const char *mainargs)
{
    int blk;

    (void)mainargs;
    printf("SDRAM mem-test: %lu MB total, 1 MB/block\n",
           (unsigned long)(SDRAM_SIZE / (1024 * 1024)));

    for (blk = 0; blk < (int)BLOCK_COUNT; blk++) {
        uintptr_t base = SDRAM_BASE + (uintptr_t)blk * BLOCK_SIZE;

        printf("  Block %2d/%2d [0x%08lx-0x%08lx] ",
               blk + 1, (int)BLOCK_COUNT,
               (unsigned long)base,
               (unsigned long)(base + BLOCK_SIZE - 1));

        test_u8_block(base, BLOCK_SIZE);
        test_u16_block(base, BLOCK_SIZE);
        test_u32_block(base, BLOCK_SIZE);
        test_u64_block(base, BLOCK_SIZE);

        if (DIAG_BLOCK(blk)) {
            volatile uint32_t *p32 = (volatile uint32_t *)base;
            int nwords = (int)WORDS_PER_BLOCK;
            test_u32_allbits(p32);
            test_u32_checkerboard(p32, nwords);
            test_u32_ones_zeros(p32, nwords);
        }

        printf("PASS\n");
    }

    printf("SDRAM mem-test: ALL %d BLOCKS PASSED\n", (int)BLOCK_COUNT);
    halt(HALT_OK);
    return 0;
}
