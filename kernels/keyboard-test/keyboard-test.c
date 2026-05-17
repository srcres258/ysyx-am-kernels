#include <am.h>
#include <klib.h>
#include <klib-macros.h>

// Auto-generated key name table using AM_KEYS macro
#define NAMEINIT(key) [AM_KEY_##key] = #key,
static const char *key_names[] = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAMEINIT)
};

static void uart_putc(char c) {
  io_write(AM_UART_TX, c);
}

static void uart_puts(const char *s) {
  while (*s) uart_putc(*s++);
}

static void uart_puthex(uint8_t v) {
  static const char hex[] = "0123456789ABCDEF";
  uart_putc(hex[v >> 4]);
  uart_putc(hex[v & 0xf]);
}

int main(const char *args) {
  ioe_init();

  bool has_kbd = io_read(AM_INPUT_CONFIG).present;
  uart_puts("=== PS/2 Keyboard Test ===\r\n");
  uart_puts(has_kbd ? "Keyboard: present\r\n" : "Keyboard: NOT present\r\n");
  uart_puts("Press any key in the PS/2 Keyboard area of NVBoard...\r\n\r\n");

  while (1) {
    AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
    if (ev.keycode == AM_KEY_NONE) continue;

    uart_puts(ev.keydown ? "DOWN: " : "UP:   ");
    uart_puts("keycode=");
    uart_puthex((uint8_t)(ev.keycode));
    uart_puts(" name=");
    if ((size_t)ev.keycode < sizeof(key_names)/sizeof(key_names[0])
        && key_names[ev.keycode])
      uart_puts(key_names[ev.keycode]);
    else
      uart_puts("UNKNOWN");
    uart_puts("\r\n");
  }

  return 0;
}
