#include <stdint.h>
#include <klib.h>

#define MEM_TEST_ADDR 0xa0000000
#define MEM_TEST_SIZE 0x1000
#define NWORDS        (MEM_TEST_SIZE / 4)

// Halt codes for each test
enum { HALT_OK = 0, HALT_U32 = 1 };

static void fail_u32(void) { halt(HALT_U32); }

static void test_u32_allbits(volatile uint32_t *p) {
    uint32_t val = 1;
    for (int i = 0; i < 32; i++) { p[i] = val; val <<= 1; }
    p[32] = 0x55555555;
    p[33] = 0xAAAAAAAA;
    p[34] = 0xFFFFFFFF;
    p[35] = 0x00000000;

    val = 1;
    for (int i = 0; i < 32; i++) {
        if (p[i] != val) fail_u32();
        val <<= 1;
    }
    if (p[32] != 0x55555555) fail_u32();
    if (p[33] != 0xAAAAAAAA) fail_u32();
    if (p[34] != 0xFFFFFFFF) fail_u32();
    if (p[35] != 0x00000000) fail_u32();
}

static void test_u32_checkerboard(volatile uint32_t *p, int n) {
    for (int i = 0; i < n; i++)
        p[i] = (i & 1) ? 0xAAAAAAAA : 0x55555555;
    for (int i = 0; i < n; i++)
        if (p[i] != (uint32_t)((i & 1) ? 0xAAAAAAAA : 0x55555555))
            fail_u32();
}

static void test_u32_addr_as_data(volatile uint32_t *p, int n) {
    uint32_t base = (uint32_t)(uintptr_t)p;
    for (int i = 0; i < n; i++)
        p[i] = base + i * 4;
    for (int i = 0; i < n; i++)
        if (p[i] != base + i * 4)
            fail_u32();
}

static void test_u32_ones_zeros(volatile uint32_t *p, int n) {
    for (int i = 0; i < n; i++) p[i] = 0xFFFFFFFF;
    for (int i = 0; i < n; i++)
        if (p[i] != 0xFFFFFFFF) fail_u32();
    for (int i = 0; i < n; i++) p[i] = 0x00000000;
    for (int i = 0; i < n; i++)
        if (p[i] != 0x00000000) fail_u32();
}

int main(const char *mainargs) {
    (void)mainargs;
    volatile uint32_t *p32 = (volatile uint32_t *)MEM_TEST_ADDR;

    // 32-bit word tests (all 4KB)
    test_u32_allbits(p32);
    test_u32_checkerboard(p32, NWORDS);
    test_u32_addr_as_data(p32, NWORDS);
    test_u32_ones_zeros(p32, NWORDS);

    halt(HALT_OK);
    return 0;
}
