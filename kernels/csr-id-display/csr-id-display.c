#include <stdint.h>
#include <am.h>

// GPIO 基址 (ysyxSoC 地址空间)
#define GPIO_BASE 0x10002000L

// GPIO 寄存器偏移
#define GPIO_SEG_OFFSET  0x8   // 7段数码管控制寄存器

// 向 GPIO 寄存器写入数据
static inline void gpio_write(uint32_t offset, uint32_t data) {
    *(volatile uint32_t *)(GPIO_BASE + offset) = data;
}

int main(const char *args) {
    uint32_t marchid;

    // 读取学号 CSR (marchid)
    asm volatile("csrr %0, marchid" : "=r" (marchid));

    // 将学号的十六进制值写入数码管寄存器
    // GPIO SEG 寄存器为 32 位, 每 4 位驱动一个 7 段数码管
    // marchid (0x017E8A6E) 的每 4 位恰好对应一个十六进制数字
    // SEG7=0, SEG6=1, SEG5=7, SEG4=E, SEG3=8, SEG2=A, SEG1=6, SEG0=E
    gpio_write(GPIO_SEG_OFFSET, marchid);

    // 死循环, 保持显示
    while (1);

    return 0;
}
