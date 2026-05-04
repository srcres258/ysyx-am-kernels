#include <stdint.h>
#include <klib.h>

int main() {
    uint32_t mvendorid, marchid;

    asm volatile("csrr %0, mvendorid" : "=r" (mvendorid));
    asm volatile("csrr %0, marchid" : "=r" (marchid));

    printf("mvendorid: 0x%08x\n", mvendorid);
    printf("marchid: %u\n", marchid);

    return 0;
}
