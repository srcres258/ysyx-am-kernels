#include <klib.h>

#define SDRAM_BASE 0xa0000000L
#define TEST_SIZE   0x1000

int main() {
  volatile unsigned int *p = (volatile unsigned int *)SDRAM_BASE;
  int nwords = TEST_SIZE / 4;

  for (int i = 0; i < nwords; i++)
    p[i] = 1u << (i % 32);
  for (int i = 0; i < nwords; i++)
    if (p[i] != (1u << (i % 32))) halt(1);

  for (int i = 0; i < nwords; i++)
    p[i] = (i & 1) ? 0xAAAAAAAA : 0x55555555;
  for (int i = 0; i < nwords; i++)
    if (p[i] != (unsigned int)((i & 1) ? 0xAAAAAAAA : 0x55555555)) halt(2);

  unsigned int base = SDRAM_BASE;
  for (int i = 0; i < nwords; i++)
    p[i] = base + i * 4;
  for (int i = 0; i < nwords; i++)
    if (p[i] != base + i * 4) halt(3);

  for (int i = 0; i < nwords; i++) p[i] = 0xFFFFFFFF;
  for (int i = 0; i < nwords; i++)
    if (p[i] != 0xFFFFFFFF) halt(4);

  for (int i = 0; i < nwords; i++) p[i] = 0x00000000;
  for (int i = 0; i < nwords; i++)
    if (p[i] != 0x00000000) halt(5);

  halt(0);
  return 0;
}
