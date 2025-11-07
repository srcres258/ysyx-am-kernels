#include <stdint.h>
#include <am.h>
#include <klib-macros.h>

static inline uint32_t inl(uintptr_t addr) { return *(volatile uint32_t *)addr; }

static inline void outl(uintptr_t addr, uint32_t data) { *(volatile uint32_t *)addr = data; }

#define FLASH_ADDR 0x30000000
#define FLASH_DATA_LEN 0x30

#define SRAM_ADDR 0x0f000000

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

/* 完成 flash 的 XIP 功能后, flash_read 函数就不需要了. */

// uint32_t flash_read(uint32_t addr) {
//     uint32_t csr, val, csr_recv, result;
//     uint64_t recv;

//     outl(SPI_CDR_ADDR, 1);
//     csr  = 1 << 13; // 启用 ASS
//     csr |= 1 << 9; // 启用 Rx_NEG
//     csr |= 64;
//     outl(SPI_CSR_ADDR, csr);
//     outl(SPI_SSR_ADDR, 1);

//     val  = 0x03 << 24;
//     val |= addr;
//     outl(SPI_TX1_ADDR, val);
//     csr |= 1 << 8; // 启用 GO_BSY
//     outl(SPI_CSR_ADDR, csr);
//     do {
//         csr_recv = inl(SPI_CSR_ADDR);
//     } while (csr_recv & (1 << 8));
//     recv  = inl(SPI_RX1_ADDR);
//     recv <<= 32;
//     recv |= inl(SPI_RX0_ADDR);
//     recv >>= 1;
//     result  = (recv & 0x000000FF) << 24;
//     result |= (recv & 0x0000FF00) << 8;
//     result |= (recv & 0x00FF0000) >> 8;
//     result |= (recv & 0xFF000000) >> 24;

//     return result;
// }

int main(const char *mainargs) {
    uint32_t val, i;
    void *sram_ptr;

    putstr("Loading program from flash to SRAM...\n");
    for (i = 0; i < FLASH_DATA_LEN / sizeof(uint32_t); i++) {
        // val = flash_read(i * sizeof(uint32_t));
        val = inl(i * sizeof(uint32_t) + FLASH_ADDR);
        sram_ptr = (void *) (SRAM_ADDR + i * sizeof(uint32_t));
        *((volatile uint32_t *) sram_ptr) = val;
    }

    putstr("Jumping into SRAM to execute program...\n");
    ((void (*)(void)) SRAM_ADDR)();

    return 0;
}
