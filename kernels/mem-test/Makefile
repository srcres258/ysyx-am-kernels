NAME = mem-test
SRCS = mem-test.c
CFLAGS = -O2 -Wno-implicit-function-declaration

# mem-test 测试 SDRAM 空间，程序本身不能加载到 SDRAM 中。
# 使用 USE_PSRAM=1 将程序加载到 PSRAM (0x80000000)。
# mainargs 格式: "bit_ext=1 word_ext=1" 指定硬件配置。
# 推荐用法:
#   make ARCH=riscv32e-ysyxsoc run USE_PSRAM=1 \
#        mainargs="bit_ext=1 word_ext=1"

include $(AM_HOME)/Makefile
