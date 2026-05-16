/*
 * SDRAM 完整访存测试 (mem-test)
 *
 * 测试对象: MT48LC16M16A2 SDRAM 颗粒 (32 MB)
 *   地址范围: 0xa0000000 ~ 0xa1ffffff
 *   容量: 4 bank x 8192 row x 512 col x 16 bit = 256 Mbit = 32 MB
 *   耗时: ~1 小时 (Verilator 仿真环境)
 *
 * 测试方法 (ysyx B2 阶段 1.8 节):
 *   1. 多宽度: 8 / 16 / 32 / 64-bit, 数据模式 data = addr & width_mask
 *   2. 两趟法: 全写 -> fence 写屏障 -> 全读校验
 *   3. 1 MB 分块 (共 32 块), 每块完成后 printf 进度
 *   4. 首/中/尾块额外执行 allbits / checkerboard / ones_zeros 诊断
 *
 * CLINT 重叠处理:
 *   地址 0xa0000048 ~ 0xa000004f 为 CLINT mtime 寄存器,
 *   与 SDRAM 地址空间重叠, 由 CPU 核心通过 DPI-C 内部拦截,
 *   不会到达 SDRAM 控制器. 测试时跳过该 8 字节范围.
 */

#include <stdint.h>
#include <am.h>
#include <klib.h>

#define SDRAM_BASE      0xa0000000UL
#define SDRAM_SIZE      (32UL * 1024 * 1024)
#define BLOCK_SIZE      (1UL  * 1024 * 1024)
#define BLOCK_COUNT     (SDRAM_SIZE / BLOCK_SIZE)

#define CLINT_ADDR      0xa0000048UL
#define CLINT_SIZE      8UL

#define WORDS_PER_BLOCK (BLOCK_SIZE / 4)
#define DIAG_BLOCK(b)   ((b) == 0 || (b) == BLOCK_COUNT / 2 || (b) == (int)(BLOCK_COUNT - 1))

enum {
    HALT_OK        = 0,
    HALT_U8_FAIL   = 1,
    HALT_U16_FAIL  = 2,
    HALT_U32_FAIL  = 3,
    HALT_U64_FAIL  = 4,
    HALT_ALLBITS   = 5,
    HALT_CHECKER   = 6,
    HALT_ONES      = 7,
};

/** Check whether [addr, addr + byte_len) overlaps CLINT mtime registers. */
static inline int clint_overlap(uintptr_t addr, size_t byte_len)
{
    uintptr_t end = addr + byte_len;
    return !(end <= CLINT_ADDR || addr >= CLINT_ADDR + CLINT_SIZE);
}

static void test_u8_block(uintptr_t base, size_t size)
{
    volatile uint8_t *p = (volatile uint8_t *)base;
    size_t i;

    for (i = 0; i < size; i++) {
        uintptr_t addr = base + i;
        if (clint_overlap(addr, 1))
            continue;
        p[i] = (uint8_t)(addr & 0xff);
    }
    asm volatile("fence" ::: "memory");

    for (i = 0; i < size; i++) {
        uintptr_t addr = base + i;
        if (clint_overlap(addr, 1))
            continue;
        uint8_t exp = (uint8_t)(addr & 0xff);
        uint8_t got = p[i];
        if (got != exp) {
            printf("FAIL u8 @0x%08lx: exp %02x got %02x (xor=%02x)\n",
                   (unsigned long)addr, exp, got, exp ^ got);
            halt(HALT_U8_FAIL);
        }
    }
}

static void test_u16_block(uintptr_t base, size_t size)
{
    volatile uint16_t *p = (volatile uint16_t *)base;
    size_t n = size / 2;
    size_t i;

    for (i = 0; i < n; i++) {
        uintptr_t addr = base + i * 2;
        if (clint_overlap(addr, 2))
            continue;
        p[i] = (uint16_t)(addr & 0xffff);
    }
    asm volatile("fence" ::: "memory");

    for (i = 0; i < n; i++) {
        uintptr_t addr = base + i * 2;
        if (clint_overlap(addr, 2))
            continue;
        uint16_t exp = (uint16_t)(addr & 0xffff);
        uint16_t got = p[i];
        if (got != exp) {
            printf("FAIL u16 @0x%08lx: exp %04x got %04x (xor=%04x)\n",
                   (unsigned long)addr, exp, got, exp ^ got);
            halt(HALT_U16_FAIL);
        }
    }
}

static void test_u32_block(uintptr_t base, size_t size)
{
    volatile uint32_t *p = (volatile uint32_t *)base;
    size_t n = size / 4;
    size_t i;

    for (i = 0; i < n; i++) {
        uintptr_t addr = base + i * 4;
        if (clint_overlap(addr, 4))
            continue;
        p[i] = (uint32_t)addr;
    }
    asm volatile("fence" ::: "memory");

    for (i = 0; i < n; i++) {
        uintptr_t addr = base + i * 4;
        if (clint_overlap(addr, 4))
            continue;
        uint32_t exp = (uint32_t)addr;
        uint32_t got = p[i];
        if (got != exp) {
            printf("FAIL u32 @0x%08lx: exp %08lx got %08lx (xor=%08lx)\n",
                   (unsigned long)addr,
                   (unsigned long)exp, (unsigned long)got,
                   (unsigned long)(exp ^ got));
            halt(HALT_U32_FAIL);
        }
    }
}

/*
 * 64-bit 访存测试
 *
 * RV32E 不支持原生 64-bit load/store 指令.
 * 编译器自动将 uint64_t 访问拆为两次 32-bit 访存,
 * volatile 限定符保证每次拆分访问独立发生、不被优化合并.
 */
static void test_u64_block(uintptr_t base, size_t size)
{
    volatile uint64_t *p = (volatile uint64_t *)base;
    size_t n = size / 8;
    size_t i;

    for (i = 0; i < n; i++) {
        uintptr_t addr = base + i * 8;
        if (clint_overlap(addr, 8))
            continue;
        p[i] = (uint64_t)addr;
    }
    asm volatile("fence" ::: "memory");

    for (i = 0; i < n; i++) {
        uintptr_t addr = base + i * 8;
        if (clint_overlap(addr, 8))
            continue;
        uint64_t exp = (uint64_t)addr;
        uint64_t got = p[i];
        if (got != exp) {
            printf("FAIL u64 @0x%08lx: "
                   "exp %08lx_%08lx got %08lx_%08lx\n",
                   (unsigned long)addr,
                   (unsigned long)((uint32_t)(exp >> 32)),
                   (unsigned long)((uint32_t)exp),
                   (unsigned long)((uint32_t)(got >> 32)),
                   (unsigned long)((uint32_t)got));
            halt(HALT_U64_FAIL);
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

    asm volatile("fence" ::: "memory");

    val = 1;
    for (i = 0; i < 32; i++) {
        if (p[i] != val) {
            printf("FAIL allbits[%d]: exp %08lx got %08lx\n",
                   i, (unsigned long)val, (unsigned long)p[i]);
            halt(HALT_ALLBITS);
        }
        val <<= 1;
    }
    if (p[32] != 0x55555555) printf("FAIL allbits[32]\n"), halt(HALT_ALLBITS);
    if (p[33] != 0xAAAAAAAA) printf("FAIL allbits[33]\n"), halt(HALT_ALLBITS);
    if (p[34] != 0xFFFFFFFF) printf("FAIL allbits[34]\n"), halt(HALT_ALLBITS);
    if (p[35] != 0x00000000) printf("FAIL allbits[35]\n"), halt(HALT_ALLBITS);
}

static void test_u32_checkerboard(volatile uint32_t *p, int n)
{
    int i;

    for (i = 0; i < n; i++)
        p[i] = (i & 1) ? 0xAAAAAAAA : 0x55555555;

    asm volatile("fence" ::: "memory");

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
    asm volatile("fence" ::: "memory");
    for (i = 0; i < n; i++) {
        if (p[i] != 0xFFFFFFFF) {
            printf("FAIL ones[%d]: got %08lx\n", i, (unsigned long)p[i]);
            halt(HALT_ONES);
        }
    }

    for (i = 0; i < n; i++) p[i] = 0x00000000;
    asm volatile("fence" ::: "memory");
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

    printf("========================================\n");
    printf("SDRAM mem-test: %lu MB total, 1 MB/block\n",
           (unsigned long)(SDRAM_SIZE / (1024 * 1024)));
    printf("CLINT mtime @0x%08lx (skip %lu bytes)\n",
           (unsigned long)CLINT_ADDR, (unsigned long)CLINT_SIZE);
    printf("========================================\n");

    for (blk = 0; blk < (int)BLOCK_COUNT; blk++) {
        uintptr_t base = SDRAM_BASE + (uintptr_t)blk * BLOCK_SIZE;

        printf("Block %2d/%2d [0x%08lx-0x%08lx] ",
               blk + 1, (int)BLOCK_COUNT,
               (unsigned long)base,
               (unsigned long)(base + BLOCK_SIZE - 1));

        test_u8_block(base, BLOCK_SIZE);
        test_u16_block(base, BLOCK_SIZE);
        test_u32_block(base, BLOCK_SIZE);
        test_u64_block(base, BLOCK_SIZE);

        if (DIAG_BLOCK(blk)) {
            volatile uint32_t *p32 = (volatile uint32_t *)base;
            test_u32_allbits(p32);
            test_u32_checkerboard(p32, (int)WORDS_PER_BLOCK);
            test_u32_ones_zeros(p32, (int)WORDS_PER_BLOCK);
        }

        printf("PASS\n");
    }

    printf("========================================\n");
    printf("SDRAM mem-test: ALL %d BLOCKS PASSED\n", (int)BLOCK_COUNT);
    printf("========================================\n");
    halt(HALT_OK);
    return 0;
}
