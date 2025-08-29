#include <amtest.h>

#define NAMEINIT(key)  [ AM_KEY_##key ] = #key,
static const char *names[] = {
  AM_KEYS(NAMEINIT)
};
static const size_t names_count = sizeof(names) / sizeof(names[0]);

static bool has_uart, has_kbd;

static void drain_keys() {
  if (has_uart) {
    while (1) {
      char ch = io_read(AM_UART_RX).data;
      if (ch == (char)-1) break;
      printf("Got (uart): %c (%d)\n", ch, ch & 0xff);
    }
  }

  if (has_kbd) {
    while (1) {
      AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
      if (ev.keycode == AM_KEY_NONE) break;
      if (ev.keycode < names_count) {
        printf("Got  (kbd): %s (%d) %s\n", names[ev.keycode], ev.keycode, ev.keydown ? "DOWN" : "UP");
      } else {
        printf("Got  (kbd): Unknown keycode (%d) %s\n", ev.keycode, ev.keydown ? "DOWN" : "UP");
      }
    }
  }
}

void keyboard_test() {
  printf("Try to press any key (uart or keyboard)...\n");
  has_uart = io_read(AM_UART_CONFIG).present;
  if (has_uart) {
    printf("UART present, waiting for input...\n");
  }
  has_kbd  = io_read(AM_INPUT_CONFIG).present;
  if (has_kbd) {
    printf("Keyboard present, waiting for input...\n");
  }
  while (1) {
    drain_keys();
  }
}
