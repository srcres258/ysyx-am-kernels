#include <stdint.h>
#include <stdbool.h>
#include <klib.h>

#define MEM_TEST_ADDR 0x80000000
#define MEM_TEST_SIZE 0x1000
#define BLOCK_SIZE    0x100

static bool test_u8_walking_ones(void *base) {
    volatile uint8_t *p = (volatile uint8_t *)base;
    for (int i = 0; i < 8; i++) p[i] = 1 << i;
    for (int i = 0; i < 8; i++) {
        uint8_t expected = 1 << i, got = p[i];
        if (got != expected) {
            printf("  FAIL: walking-1 u8 @ 0x%08x: expected 0x%02x, got 0x%02x\n",
                   (uint32_t)(uintptr_t)(p + i), expected, got);
            return false;
        }
    }
    return true;
}

static bool test_u8_walking_zeros(void *base) {
    volatile uint8_t *p = (volatile uint8_t *)base;
    for (int i = 0; i < 8; i++) p[i] = ~(uint8_t)(1 << i);
    for (int i = 0; i < 8; i++) {
        uint8_t expected = ~(uint8_t)(1 << i), got = p[i];
        if (got != expected) {
            printf("  FAIL: walking-0 u8 @ 0x%08x: expected 0x%02x, got 0x%02x\n",
                   (uint32_t)(uintptr_t)(p + i), expected, got);
            return false;
        }
    }
    return true;
}

static bool test_u8_checkerboard(void *base, int nbytes) {
    volatile uint8_t *p = (volatile uint8_t *)base;
    for (int i = 0; i < nbytes; i++) p[i] = (i & 1) ? 0xAA : 0x55;
    for (int i = 0; i < nbytes; i++) {
        uint8_t expected = (i & 1) ? 0xAA : 0x55, got = p[i];
        if (got != expected) {
            printf("  FAIL: checkerboard u8 @ 0x%08x: expected 0x%02x, got 0x%02x\n",
                   (uint32_t)(uintptr_t)(p + i), expected, got);
            return false;
        }
    }
    return true;
}

static bool test_u8_addr_as_data(void *base, int nbytes) {
    volatile uint8_t *p = (volatile uint8_t *)base;
    uint32_t b = (uint32_t)(uintptr_t)base;
    for (int i = 0; i < nbytes; i++) p[i] = (uint8_t)((b + i) & 0xFF);
    for (int i = 0; i < nbytes; i++) {
        uint8_t expected = (uint8_t)((b + i) & 0xFF), got = p[i];
        if (got != expected) {
            printf("  FAIL: addr-as-data u8 @ 0x%08x: expected 0x%02x, got 0x%02x\n",
                   (uint32_t)(uintptr_t)(p + i), expected, got);
            return false;
        }
    }
    return true;
}

static bool test_u16_allbits(void *base) {
    volatile uint16_t *p = (volatile uint16_t *)base;
    static const uint16_t vals[] = {
        0x0001, 0x0002, 0x0004, 0x0008,
        0x0010, 0x0020, 0x0040, 0x0080,
        0x0100, 0x0200, 0x0400, 0x0800,
        0x1000, 0x2000, 0x4000, 0x8000,
        0x5555, 0xAAAA, 0xFFFF, 0x0000,
    };
    int n = sizeof(vals) / sizeof(vals[0]);

    for (int i = 0; i < n; i++) p[i] = vals[i];
    for (int i = 0; i < n; i++) {
        uint16_t got = p[i];
        if (got != vals[i]) {
            printf("  FAIL: u16 @ 0x%08x: expected 0x%04x, got 0x%04x\n",
                   (uint32_t)(uintptr_t)(p + i), vals[i], got);
            return false;
        }
    }
    return true;
}

static bool test_u32_allbits(void *base) {
    volatile uint32_t *p = (volatile uint32_t *)base;

    uint32_t val = 1;
    for (int i = 0; i < 32; i++) { p[i] = val; val <<= 1; }
    p[32] = 0x55555555;
    p[33] = 0xAAAAAAAA;
    p[34] = 0xFFFFFFFF;
    p[35] = 0x00000000;

    val = 1;
    for (int i = 0; i < 32; i++) {
        uint32_t got = p[i];
        if (got != val) {
            printf("  FAIL: u32 walking-1 @ 0x%08x: expected 0x%08x, got 0x%08x\n",
                   (uint32_t)(uintptr_t)(p + i), val, got);
            return false;
        }
        val <<= 1;
    }

    static const uint32_t rest[] = {0x55555555, 0xAAAAAAAA, 0xFFFFFFFF, 0x00000000};
    static const char *rest_names[] = {"0x55555555", "0xAAAAAAAA", "0xFFFFFFFF", "0x00000000"};
    for (int i = 0; i < 4; i++) {
        uint32_t got = p[32 + i];
        if (got != rest[i]) {
            printf("  FAIL: u32 %s @ 0x%08x: got 0x%08x\n",
                   rest_names[i], (uint32_t)(uintptr_t)(p + 32 + i), got);
            return false;
        }
    }
    return true;
}

int main(const char *mainargs) {
    (void)mainargs;
    void *end = (void *)(MEM_TEST_ADDR + MEM_TEST_SIZE - 1);
    int total = MEM_TEST_SIZE / BLOCK_SIZE;
    int block = 0;

    printf("=== PSRAM mem-test ===\n");
    printf("Base: 0x%08x  Size: %d bytes  Blocks: %d x %d bytes\n\n",
           MEM_TEST_ADDR, MEM_TEST_SIZE, total, BLOCK_SIZE);

    for (void *cur = (void *)MEM_TEST_ADDR; cur <= end; cur += BLOCK_SIZE) {
        printf("Block %d/%d @ 0x%08x:\n",
               block + 1, total, (uint32_t)(uintptr_t)cur);

        if (!test_u8_walking_ones(cur))        goto fail;
        printf("  walking-1 u8 ... PASS\n");

        if (!test_u8_walking_zeros(cur))       goto fail;
        printf("  walking-0 u8 ... PASS\n");

        if (!test_u8_checkerboard(cur, BLOCK_SIZE)) goto fail;
        printf("  checkerboard u8 ... PASS\n");

        if (!test_u8_addr_as_data(cur, BLOCK_SIZE)) goto fail;
        printf("  addr-as-data u8 ... PASS\n");

        if (!test_u16_allbits(cur))            goto fail;
        printf("  allbits u16 ... PASS\n");

        if (!test_u32_allbits(cur))            goto fail;
        printf("  allbits u32 ... PASS\n");

        block++;
    }

    printf("\n=== ALL TESTS PASSED ===\n");
    return 0;

fail:
    printf("\n=== TEST FAILED ===\n");
    return 1;
}
