#include <stdint.h>
#include <am.h>
#include <klib-macros.h>

// GPIO 基址 (ysyxSoC 地址空间)
#define GPIO_BASE 0x10002000L

// GPIO 寄存器偏移
#define GPIO_LED_OFFSET  0x0   // LED 输出寄存器
#define GPIO_SW_OFFSET   0x4   // 拨码开关输入寄存器
#define GPIO_SEG_OFFSET  0x8   // 7段数码管控制寄存器

static inline uint32_t gpio_read(uint32_t offset) {
    return *(volatile uint32_t *)(GPIO_BASE + offset);
}

static inline void gpio_write(uint32_t offset, uint32_t data) {
    *(volatile uint32_t *)(GPIO_BASE + offset) = data;
}

// 软件延迟 (忙等待循环)
static void delay(int cycles) {
    for (volatile int i = 0; i < cycles; i++) {
        // 空循环
    }
}

int main(const char *args) {
    // 初始状态: LED 全灭
    gpio_write(GPIO_LED_OFFSET, 0x0000);

    // 流水灯方向: 1 = 从左到右, 0 = 从右到左
    int direction = 1;
    // 当前点亮的位置 (0 表示 LD0, 15 表示 LD15)
    int pos = 0;

    while (1) {
        // 点亮当前位置的 LED
        gpio_write(GPIO_LED_OFFSET, (uint32_t)(1 << pos));

        // 延时 (调节延时大小可以改变流水速度)
        delay(500);

        // 更新位置
        if (direction) {
            pos++;
            if (pos >= 15) direction = 0;  // 到最右边, 反向
        } else {
            pos--;
            if (pos <= 0) direction = 1;   // 到最左边, 反向
        }
    }

    return 0;
}
