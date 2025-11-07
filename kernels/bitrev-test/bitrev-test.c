#include <stdbool.h>
#include <am.h>
#include <klib-macros.h>

static inline uint32_t inl(uintptr_t addr) { return *(volatile uint32_t *)addr; }

static inline void outl(uintptr_t addr, uint32_t data) { *(volatile uint32_t *)addr = data; }

#define SPI_ADDR 0x10001000
#define SPI_LEN  0x1000

#define SPI_RX0_ADDR SPI_ADDR
#define SPI_RX1_ADDR (SPI_ADDR + 0x04)
#define SPI_RX2_ADDR (SPI_ADDR + 0x08)
#define SPI_RX3_ADDR (SPI_ADDR + 0x0c)
#define SPI_TX0_ADDR SPI_ADDR
#define SPI_TX1_ADDR (SPI_ADDR + 0x04)
#define SPI_TX2_ADDR (SPI_ADDR + 0x08)
#define SPI_TX3_ADDR (SPI_ADDR + 0x0c)
#define SPI_CSR_ADDR (SPI_ADDR + 0x10)
#define SPI_CDR_ADDR (SPI_ADDR + 0x14)
#define SPI_SSR_ADDR (SPI_ADDR + 0x18)

int main(const char *mainargs) {
    uint32_t csr, csr_recv, val, bitrev_val, bitrev_val_recv;
    int i;
    bool pass;

    outl(SPI_CDR_ADDR, 1);
    csr = 0;
    csr |= 1 << 13; // 启用 ASS
    // csr |= 1 << 11; // 启用 LSB
    csr |= 1 << 9; // 启用 Rx_NEG
    csr |= 16;
    outl(SPI_CSR_ADDR, csr);
    outl(SPI_SSR_ADDR, 1 << 7);

    val = 0b00111000;
    bitrev_val = 0b00011100;
    outl(SPI_TX0_ADDR, val << 8);
    csr |= 1 << 8; // 启用 GO_BSY
    outl(SPI_CSR_ADDR, csr);
    do {
        csr_recv = inl(SPI_CSR_ADDR);
    } while (csr_recv & (1 << 8));
    bitrev_val_recv = inl(SPI_RX0_ADDR);
    bitrev_val_recv &= 0xFF;
    // bitrev_val_recv >>= 8;
    pass = bitrev_val_recv == bitrev_val;
    putstr(pass ? "PASS\n" : "FAIL\n");
    i = 7;
    while (1) {
        putch(bitrev_val_recv & (1 << i) ? '1' : '0');
        if (i > 0) {
            i--;
        } else {
            break;
        }
    };
    putch('\n');

    return pass ? 0 : 1;
}
