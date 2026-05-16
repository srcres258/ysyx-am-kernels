/*
 * SDRAM 完整访存测试 (mem-test) v2 — 支持动态配置
 *
 * 测试对象: MT48LC16M16A2 SDRAM 颗粒
 *   1颗粒 (无扩展):     32MB,  addr: 0xa0000000 ~ 0xa1ffffff
 *   2颗粒 (位扩展):     64MB,  addr: 0xa0000000 ~ 0xa3ffffff
 *   4颗粒 (位+字扩展):  128MB, addr: 0xa0000000 ~ 0xa7ffffff
 *
 * 测试方法 (ysyx B2 阶段 1.8 节):
 *   1. 多宽度: 8 / 16 / 32 / 64-bit, 数据模式 data = addr & width_mask
 *   2. 两趟法: 全写 -> fence 写屏障 -> 全读校验
 *   3. 1 MB 分块, 每块完成后输出进度
 *   4. 首/中/尾块额外执行 allbits / checkerboard / ones_zeros 诊断
 *
 * CLINT 重叠处理:
 *   地址 0xa0000048 ~ 0xa000004f 为 CLINT mtime 寄存器,
 *   与 SDRAM 地址空间重叠, 由 CPU 核心通过 DPI-C 内部拦截,
 *   不会到达 SDRAM 控制器. 测试时跳过该 8 字节范围.
 *
 * mainargs 格式: "BIT=1 WORD=1"
 *   BIT:  0 = 16-bit single,   1 = 32-bit bit-extension  (default: 1)
 *   WORD: 0 = single channel,  1 = word-extension       (default: 1)
 *
 * 注意:
 *   AM klib 的 printf 在 RV32E 上对 %2d / %08lx / %02x 等带宽度/补零的
 *   格式说明符有已知问题, 会输出错误数字. 本代码完全避免使用 printf 的
 *   数字格式化功能, 改用 putch / puts 组合输出.
 */

#include <stdint.h>
#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#define SDRAM_BASE          0xa0000000UL
#define BLOCK_SIZE          (1UL * 1024 * 1024)
#define CLINT_ADDR          0xa0000048UL
#define CLINT_SIZE          8UL
#define PROGRESS_STEP       (4096UL)
#define PROGRESS_DOTS_WRAP  64

static size_t sdram_size;
static int    block_count;

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

static inline int diag_block(int b)
{
    return b == 0 || b == block_count / 2 || b == block_count - 1;
}

static inline int clint_overlap(uintptr_t addr, size_t byte_len)
{
    uintptr_t end = addr + byte_len;
    return !(end <= CLINT_ADDR || addr >= CLINT_ADDR + CLINT_SIZE);
}

static inline void progress_dot(int *pline)
{
    putch('.');
    if (++(*pline) >= PROGRESS_DOTS_WRAP) {
        putstr("\n      ");
        *pline = 0;
    }
}

#define PROGRESS_MAYBE(_bytes, _done, _next, _pline) do { \
    (_done) += (_bytes);                                   \
    if ((_done) >= (_next)) {                              \
        progress_dot(_pline);                              \
        (_next) += PROGRESS_STEP;                          \
    }                                                      \
} while (0)

static void print_hex_nibbles(unsigned int value, int nibbles)
{
    int shift;
    for (shift = (nibbles - 1) * 4; shift >= 0; shift -= 4)
        putch("0123456789abcdef"[(value >> shift) & 0xf]);
}

static void print_small_int(int value)
{
    if (value >= 100)
        putch('0' + value / 100);
    if (value >= 10)
        putch('0' + (value / 10) % 10);
    putch('0' + value % 10);
}

static int parse_keyval(const char *str, const char *key, int *val_out)
{
    while (*str) {
        while (*str == ' ' || *str == '\t')
            str++;
        if (!*str)
            break;

        const char *k = key;
        while (*k && *str == *k) {
            str++;
            k++;
        }
        if (*k != '\0') {
            while (*str && *str != ' ' && *str != '\t')
                str++;
            continue;
        }

        if (*str != '=')
            return 0;
        str++;

        int value = 0;
        while (*str >= '0' && *str <= '9') {
            value = value * 10 + (*str - '0');
            str++;
        }
        *val_out = value;
        return 1;
    }
    return 0;
}

static void parse_mainargs(const char *mainargs,
                           int *bit_ext, int *word_ext)
{
    *bit_ext  = 1;
    *word_ext = 1;

    if (!mainargs || !*mainargs)
        return;

    parse_keyval(mainargs, "BIT",  bit_ext);
    parse_keyval(mainargs, "WORD", word_ext);

    if (!*bit_ext && *word_ext) {
        putstr("WARNING: BIT=0 WORD=1 invalid, "
             "falling back to WORD=0\n");
        *word_ext = 0;
    }
}

static void compute_sdram_size(int bit_ext, int word_ext)
{
    sdram_size  = 32UL * 1024 * 1024;
    if (bit_ext)  sdram_size *= 2;
    if (word_ext) sdram_size *= 2;
    block_count = (int)(sdram_size / BLOCK_SIZE);
}

/* ------------------------------------------------------------ */

static void test_u8_block(uintptr_t base, size_t size)
{
    volatile uint8_t *ptr = (volatile uint8_t *)base;
    size_t bytes_done = 0, next_dot = PROGRESS_STEP;
    int pline = 0;
    size_t i;

    for (i = 0; i < size; i++) {
        uintptr_t addr = base + i;
        if (!clint_overlap(addr, 1))
            ptr[i] = (uint8_t)(addr & 0xff);
        PROGRESS_MAYBE(1, bytes_done, next_dot, &pline);
    }
    asm volatile("fence" ::: "memory");

    for (i = 0; i < size; i++) {
        uintptr_t addr = base + i;
        if (clint_overlap(addr, 1)) {
            PROGRESS_MAYBE(1, bytes_done, next_dot, &pline);
            continue;
        }
        uint8_t expected = (uint8_t)(addr & 0xff);
        uint8_t got      = ptr[i];
        if (got != expected) {
            putstr("\nFAIL u8 @");
            print_hex_nibbles((unsigned int)addr, 8);
            putstr(" exp=");
            print_hex_nibbles(expected, 2);
            putstr(" got=");
            print_hex_nibbles(got, 2);
            putch('\n');
            halt(HALT_U8_FAIL);
        }
        PROGRESS_MAYBE(1, bytes_done, next_dot, &pline);
    }
}

static void test_u16_block(uintptr_t base, size_t size)
{
    volatile uint16_t *ptr = (volatile uint16_t *)base;
    size_t count = size / 2;
    size_t bytes_done = 0, next_dot = PROGRESS_STEP;
    int pline = 0;
    size_t i;

    for (i = 0; i < count; i++) {
        uintptr_t addr = base + i * 2;
        if (!clint_overlap(addr, 2))
            ptr[i] = (uint16_t)(addr & 0xffff);
        PROGRESS_MAYBE(2, bytes_done, next_dot, &pline);
    }
    asm volatile("fence" ::: "memory");

    for (i = 0; i < count; i++) {
        uintptr_t addr = base + i * 2;
        if (clint_overlap(addr, 2)) {
            PROGRESS_MAYBE(2, bytes_done, next_dot, &pline);
            continue;
        }
        uint16_t expected = (uint16_t)(addr & 0xffff);
        uint16_t got      = ptr[i];
        if (got != expected) {
            putstr("\nFAIL u16 @");
            print_hex_nibbles((unsigned int)addr, 8);
            putstr(" exp=");
            print_hex_nibbles(expected, 4);
            putstr(" got=");
            print_hex_nibbles(got, 4);
            putch('\n');
            halt(HALT_U16_FAIL);
        }
        PROGRESS_MAYBE(2, bytes_done, next_dot, &pline);
    }
}

static void test_u32_block(uintptr_t base, size_t size)
{
    volatile uint32_t *ptr = (volatile uint32_t *)base;
    size_t count = size / 4;
    size_t bytes_done = 0, next_dot = PROGRESS_STEP;
    int pline = 0;
    size_t i;

    for (i = 0; i < count; i++) {
        uintptr_t addr = base + i * 4;
        if (!clint_overlap(addr, 4))
            ptr[i] = (uint32_t)addr;
        PROGRESS_MAYBE(4, bytes_done, next_dot, &pline);
    }
    asm volatile("fence" ::: "memory");

    for (i = 0; i < count; i++) {
        uintptr_t addr = base + i * 4;
        if (clint_overlap(addr, 4)) {
            PROGRESS_MAYBE(4, bytes_done, next_dot, &pline);
            continue;
        }
        uint32_t expected = (uint32_t)addr;
        uint32_t got      = ptr[i];
        if (got != expected) {
            putstr("\nFAIL u32 @");
            print_hex_nibbles((unsigned int)addr, 8);
            putstr(" exp=");
            print_hex_nibbles(expected, 8);
            putstr(" got=");
            print_hex_nibbles(got, 8);
            putch('\n');
            halt(HALT_U32_FAIL);
        }
        PROGRESS_MAYBE(4, bytes_done, next_dot, &pline);
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
    volatile uint64_t *ptr = (volatile uint64_t *)base;
    size_t count = size / 8;
    size_t bytes_done = 0, next_dot = PROGRESS_STEP;
    int pline = 0;
    size_t i;

    for (i = 0; i < count; i++) {
        uintptr_t addr = base + i * 8;
        if (!clint_overlap(addr, 8))
            ptr[i] = (uint64_t)addr;
        PROGRESS_MAYBE(8, bytes_done, next_dot, &pline);
    }
    asm volatile("fence" ::: "memory");

    for (i = 0; i < count; i++) {
        uintptr_t addr = base + i * 8;
        if (clint_overlap(addr, 8)) {
            PROGRESS_MAYBE(8, bytes_done, next_dot, &pline);
            continue;
        }
        uint64_t expected = (uint64_t)addr;
        uint64_t got      = ptr[i];
        if (got != expected) {
            putstr("\nFAIL u64 @");
            print_hex_nibbles((unsigned int)addr, 8);
            putch('\n');
            halt(HALT_U64_FAIL);
        }
        PROGRESS_MAYBE(8, bytes_done, next_dot, &pline);
    }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void test_u32_allbits(volatile uint32_t *ptr)
{
    uint32_t val = 1;
    int i;

    for (i = 0; i < 32; i++) {
        ptr[i] = val;
        val <<= 1;
    }
    ptr[32] = 0x55555555;
    ptr[33] = 0xAAAAAAAA;
    ptr[34] = 0xFFFFFFFF;
    ptr[35] = 0x00000000;

    asm volatile("fence" ::: "memory");

    val = 1;
    for (i = 0; i < 32; i++) {
        if (ptr[i] != val) {
            putstr("FAIL allbits\n");
            halt(HALT_ALLBITS);
        }
        val <<= 1;
    }
    if (ptr[32] != 0x55555555) { putstr("FAIL allbits[32]\n"); halt(HALT_ALLBITS); }
    if (ptr[33] != 0xAAAAAAAA) { putstr("FAIL allbits[33]\n"); halt(HALT_ALLBITS); }
    if (ptr[34] != 0xFFFFFFFF) { putstr("FAIL allbits[34]\n"); halt(HALT_ALLBITS); }
    if (ptr[35] != 0x00000000) { putstr("FAIL allbits[35]\n"); halt(HALT_ALLBITS); }
}

static void test_u32_checkerboard(volatile uint32_t *ptr, int count)
{
    size_t bytes_done = 0, next_dot = PROGRESS_STEP;
    int pline = 0;
    int i;

    for (i = 0; i < count; i++) {
        ptr[i] = (i & 1) ? 0xAAAAAAAA : 0x55555555;
        PROGRESS_MAYBE(4, bytes_done, next_dot, &pline);
    }
    asm volatile("fence" ::: "memory");

    for (i = 0; i < count; i++) {
        uint32_t expected = (uint32_t)((i & 1) ? 0xAAAAAAAA : 0x55555555);
        if (ptr[i] != expected) {
            putstr("\nFAIL checker\n");
            halt(HALT_CHECKER);
        }
        PROGRESS_MAYBE(4, bytes_done, next_dot, &pline);
    }
}

static void test_u32_ones_zeros(volatile uint32_t *ptr, int count)
{
    size_t bytes_done = 0, next_dot = PROGRESS_STEP;
    int pline = 0;
    int i;

    for (i = 0; i < count; i++)
        ptr[i] = 0xFFFFFFFF;
    asm volatile("fence" ::: "memory");
    for (i = 0; i < count; i++) {
        if (ptr[i] != 0xFFFFFFFF) {
            putstr("\nFAIL ones\n");
            halt(HALT_ONES);
        }
        PROGRESS_MAYBE(4, bytes_done, next_dot, &pline);
    }

    bytes_done = 0;
    next_dot   = PROGRESS_STEP;
    pline      = 0;

    for (i = 0; i < count; i++)
        ptr[i] = 0x00000000;
    asm volatile("fence" ::: "memory");
    for (i = 0; i < count; i++) {
        if (ptr[i] != 0x00000000) {
            putstr("\nFAIL zeros\n");
            halt(HALT_ONES);
        }
        PROGRESS_MAYBE(4, bytes_done, next_dot, &pline);
    }
}

/* ================================================================== */

int main(const char *mainargs)
{
    int bit_ext, word_ext;
    int blk;

    parse_mainargs(mainargs, &bit_ext, &word_ext);
    compute_sdram_size(bit_ext, word_ext);

    putstr("========================================\n");
    putstr("SDRAM mem-test v2\n");
    putstr("  BIT=");
    putch('0' + bit_ext);
    putstr("  WORD=");
    putch('0' + word_ext);
    putstr("\n  Blocks: ");
    print_small_int(block_count);
    putstr("\n  Base: a0000000\n");
    putstr("========================================\n");

    for (blk = 0; blk < block_count; blk++) {
        uintptr_t base = SDRAM_BASE + (uintptr_t)blk * BLOCK_SIZE;

        putstr("B");
        print_small_int(blk + 1);
        putstr("/");
        print_small_int(block_count);
        putstr(" u8 ");
        test_u8_block(base, BLOCK_SIZE);
        putstr("OK\n");

        putstr("    u16 ");
        test_u16_block(base, BLOCK_SIZE);
        putstr("OK\n");

        putstr("    u32 ");
        test_u32_block(base, BLOCK_SIZE);
        putstr("OK\n");

        putstr("    u64 ");
        test_u64_block(base, BLOCK_SIZE);
        putstr("OK\n");

        if (diag_block(blk)) {
            volatile uint32_t *p32 = (volatile uint32_t *)base;

            putstr("    allbits    ");
            test_u32_allbits(p32);
            putstr("OK\n");

            putstr("    checker    ");
            test_u32_checkerboard(p32, (int)(BLOCK_SIZE / 4));
            putstr("OK\n");

            putstr("    ones_zeros ");
            test_u32_ones_zeros(p32, (int)(BLOCK_SIZE / 4));
            putstr("OK\n");
        }

        putstr("    => BLOCK PASS\n");
    }

    putstr("========================================\n");
    putstr("SDRAM mem-test v2: ALL BLOCKS PASSED\n");
    putstr("========================================\n");
    halt(HALT_OK);
    return 0;
}
