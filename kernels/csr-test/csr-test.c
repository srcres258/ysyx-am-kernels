#include <stdint.h>
#include <klib.h>

static void put_hex(uint32_t val) {
    static const char hex[] = "0123456789abcdef";
    for (int i = 7; i >= 0; i--) {
        putch(hex[(val >> (i * 4)) & 0xF]);
    }
}

static void put_str(const char *s) {
    for (; *s; s++) putch(*s);
}

int main() {
    uint32_t mvendorid, marchid;

    asm volatile("csrr %0, mvendorid" : "=r" (mvendorid));
    asm volatile("csrr %0, marchid" : "=r" (marchid));

    put_str("mvendorid: 0x");
    put_hex(mvendorid);
    put_str("\nmarchid: 0x");
    put_hex(marchid);
    put_str(" (expected 0x017e8a6e = 25070190)\n");

    return 0;
}
