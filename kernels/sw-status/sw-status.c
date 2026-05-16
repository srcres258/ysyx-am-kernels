// === sw-status.c — 拨码开关密码验证 + 周期性状态监控 ===
//
// 硬编码 16-bit 密码 (SW_PASSWORD), 程序启动后持续轮询 GPIO 拨码开关状态,
// 直到开关值与密码匹配才进入主循环。
// 主循环中每 500ms:
//   1. 读取全部 16 个拨码开关状态 (GPIO 偏移 0x4)
//   2. 将开关状态写入 LED 输出寄存器 (GPIO 偏移 0x0)
//   3. 通过 UART 打印二进制/十六进制开关值及各开关 ON/OFF 详情
//
// 构建:  cd am-kernels/kernels/sw-status && make ARCH=riscv32e-ysyxsoc
// 运行:  cd am-kernels/kernels/sw-status && make ARCH=riscv32e-ysyxsoc run

#include <stdint.h>
#include <am.h>
#include <klib-macros.h>

// ============================================================
//  GPIO 硬件地址 (ysyxSoC 地址空间, APB 总线)
// ============================================================
#define GPIO_BASE         0x10002000L

// GPIO 寄存器偏移
#define GPIO_LED_OFFSET   0x0    // LED 输出寄存器 (R/W, 低 16-bit 有效)
#define GPIO_SW_OFFSET    0x4    // 拨码开关输入寄存器 (R, 低 16-bit 有效)

// ============================================================
//  密码配置 (用户可修改)
//  示例: 0x55aa = 0101 0101 1010 1010
//        0x1234 = 0001 0010 0011 0100
//        0xa5a5 = 1010 0101 1010 0101
// ============================================================
#define SW_PASSWORD       0x55aa

// ============================================================
//  MMIO 辅助函数
// ============================================================
static inline uint32_t gpio_read(uint32_t offset) {
    return *(volatile uint32_t *)(GPIO_BASE + offset);
}

static inline void gpio_write(uint32_t offset, uint32_t data) {
    *(volatile uint32_t *)(GPIO_BASE + offset) = data;
}

// ============================================================
//  数值打印辅助函数
//  不使用 printf —— RV32E 上 printf 存在 %02x / %08lx 等格式化 bug
// ============================================================

// 打印 16-bit 二进制字符串 (每 4-bit 用空格分组)
static void print_sw_binary(uint32_t sw) {
    for (int i = 15; i >= 0; i--) {
        putch((sw >> i) & 1 ? '1' : '0');
        if (i == 12 || i == 8 || i == 4) putch(' ');
    }
}

// 打印 4 位十六进制 (如 "55aa")
static void print_sw_hex(uint32_t sw) {
    const char hex[] = "0123456789abcdef";
    putch(hex[(sw >> 12) & 0xf]);
    putch(hex[(sw >>  8) & 0xf]);
    putch(hex[(sw >>  4) & 0xf]);
    putch(hex[(sw >>  0) & 0xf]);
}

// 打印十进制小整数 (0 ~ 999)
static void print_small_int(int n) {
    if (n == 0) { putch('0'); return; }
    char buf[8];
    int pos = 0;
    while (n > 0) { buf[pos++] = '0' + (n % 10); n /= 10; }
    while (pos > 0) putch(buf[--pos]);
}

// ============================================================
//  定时器延时 (基于 AM IOE TIMER_UPTIME)
//  必须在 ioe_init() 之后调用
// ============================================================
static void delay_ms(int ms) {
    uint64_t start = io_read(AM_TIMER_UPTIME).us;
    uint64_t target = start + (uint64_t)ms * 1000;
    while (io_read(AM_TIMER_UPTIME).us < target) ;
}

// ============================================================
//  主函数
// ============================================================
int main(const char *args) {
    // ========================================================
    //  阶段 1: 密码验证 (无需 ioe_init, 使用 GPIO MMIO 直接访问)
    // ========================================================
    putstr("=== sw-status: Dip Switch Password Check ===\n");
    putstr("Please set switches to match the password: 0x");
    print_sw_hex(SW_PASSWORD);
    putstr(" (binary: ");
    print_sw_binary(SW_PASSWORD);
    putstr(")\n");
    putstr("Waiting for correct switch configuration...\n");

    uint32_t sw;
    do {
        sw = gpio_read(GPIO_SW_OFFSET) & 0xFFFF;
        // 每次轮询都更新 LED, 方便在 NVBoard GUI 上观察当前开关状态
        gpio_write(GPIO_LED_OFFSET, sw);
    } while (sw != SW_PASSWORD);

    putstr("PASSWORD MATCH! Starting periodic switch monitoring.\n\n");

    // ========================================================
    //  阶段 2: 初始化 IOE (供定时器使用)
    // ========================================================
    ioe_init();

    // ========================================================
    //  阶段 3: 主循环 — 周期性监控开关状态 + LED 同步 + UART 打印
    // ========================================================
    while (1) {
        sw = gpio_read(GPIO_SW_OFFSET) & 0xFFFF;

        // 将开关状态写入 LED 输出寄存器 (同步 LED 亮灭)
        gpio_write(GPIO_LED_OFFSET, sw);

        // UART 打印开关状态
        putstr("[SW-STATUS] 0x");
        print_sw_hex(sw);
        putstr(" (");
        print_sw_binary(sw);
        putstr(")  | ON: ");

        // 打印每个开关的详细 ON/OFF 状态
        int first = 1;
        for (int i = 15; i >= 0; i--) {
            if (sw & (1u << i)) {
                if (!first) putch(',');
                putstr("SW");
                print_small_int(i);
                first = 0;
            }
        }
        if (first) putstr("(none)");
        putstr("\n");

        // 延时 500ms
        delay_ms(500);
    }

    return 0;
}
