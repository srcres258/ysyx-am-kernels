/*
 * ChipLink 访存测试 (chiplink-test)
 *
 * 测试 ChipLink 地址空间首部 4 KB (0xc000_0000 ~ 0xc000_0FFF)
 * 的读/写访问是否正常。
 *
 * 注: ChipLink 的 AXI4→TL 转换路径不支持 PutPartial (子字掩码写),
 * 因此本测试仅使用 32-bit 字访问 (sw/lw), 不包含 sb/sh/lb/lh。
 *
 * 测试方法:
 *   两趟法 — 全写 1024 个 32-bit 字 → fence 屏障 → 全读校验。
 *   数据模式: data = word_address (0xc000_0000 + i*4)。
 */

#include <stdint.h>
#include <am.h>

#define CHIPLINK_MEM_BASE  0xc0000000UL
#define TEST_BYTES         4096U

int main(void) {
    uint32_t i;
    uint32_t word_count = TEST_BYTES / 4;

    /* ---- 32-bit write ---- */
    for (i = 0; i < word_count; i++) {
        uintptr_t addr = CHIPLINK_MEM_BASE + i * 4;
        *(volatile uint32_t *)addr = (uint32_t)addr;
    }

    /* fence 写屏障: 确保数据先写入, 再回读校验 */
    asm volatile("fence" ::: "memory");

    /* ---- 32-bit verify ---- */
    for (i = 0; i < word_count; i++) {
        uintptr_t addr = CHIPLINK_MEM_BASE + i * 4;
        if (*(volatile uint32_t *)addr != (uint32_t)addr) {
            halt(1);
        }
    }

    halt(0);
    return 0;
}
