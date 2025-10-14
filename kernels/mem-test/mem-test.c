#include <stdint.h>
#include <stdbool.h>

#define MEM_TEST_ADDR 0x0f000000
#define MEM_TEST_SIZE 0x1000

bool test_u8(void *addr) {
    uint8_t i;
    volatile uint8_t read_val;
    volatile uint8_t *cur;

    for (i = 0, cur = (uint8_t *) addr; ; i++, cur++) {
        *cur = i;
        read_val = *cur;
        if (read_val != i) {
            return false;
        }

        if (i == 0xFF) {
            break;
        }
    }

    return true;
}

bool test_u16(void *addr) {
    uint16_t i;
    volatile uint16_t read_val;
    volatile uint16_t *cur;

    for (i = 0, cur = (uint16_t *) addr; ; i++, cur++) {
        *cur = i;
        read_val = *cur;
        if (read_val != i) {
            return false;
        }
        if (i == 0x7F) {
            break;
        }
    }

    return true;
}

bool test_u32(void *addr) {
    uint32_t i;
    volatile uint32_t read_val;
    volatile uint32_t *cur;

    for (i = 0, cur = (uint32_t *) addr; ; i++, cur++) {
        *cur = i;
        read_val = *cur;
        if (read_val != i) {
            return false;
        }
        if (i == 0x3F) {
            break;
        }
    }

    return true;
}

int main(const char *mainargs) {
    void *cur, *end;

    end = (void *) (MEM_TEST_ADDR + MEM_TEST_SIZE - 1);
    for (
        cur = (void *) MEM_TEST_ADDR;
        cur <= end;
        cur += 0x100
    ) {
        if (!test_u8(cur)) {
            return 1;
        }
        if (!test_u16(cur)) {
            return 1;
        }
        if (!test_u32(cur)) {
            return 1;
        }
    }

    return 0;
}
