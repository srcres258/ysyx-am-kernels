#include <am.h>
#include <klib.h>
#include <klib-macros.h>

// 按键名称表 (精简版, 仅用于显示)
static const char *key_names[] = {
  [0]  = "NONE",
  [1]  = "ESCAPE",   [2]  = "F1",      [3]  = "F2",
  [4]  = "F3",       [5]  = "F4",      [6]  = "F5",
  [7]  = "F6",       [8]  = "F7",      [9]  = "F8",
  [10] = "F9",       [11] = "F10",     [12] = "F11",     [13] = "F12",
  [14] = "GRAVE",    [15] = "1",       [16] = "2",       [17] = "3",
  [18] = "4",        [19] = "5",       [20] = "6",       [21] = "7",
  [22] = "8",        [23] = "9",       [24] = "0",       [25] = "MINUS",
  [26] = "EQUALS",   [27] = "BACKSPACE",
  [28] = "TAB",      [29] = "Q",       [30] = "W",       [31] = "E",
  [32] = "R",        [33] = "T",       [34] = "Y",       [35] = "U",
  [36] = "I",        [37] = "O",       [38] = "P",       [39] = "LEFTBRACKET",
  [40] = "RIGHTBRACKET", [41] = "BACKSLASH",
  [42] = "CAPSLOCK", [43] = "A",       [44] = "S",       [45] = "D",
  [46] = "F",        [47] = "G",       [48] = "H",       [49] = "J",
  [50] = "K",        [51] = "L",       [52] = "SEMICOLON",
  [53] = "APOSTROPHE", [54] = "RETURN",
  [55] = "LSHIFT",   [56] = "Z",       [57] = "X",       [58] = "C",
  [59] = "V",        [60] = "B",       [61] = "N",       [62] = "M",
  [63] = "COMMA",    [64] = "PERIOD",  [65] = "SLASH",   [66] = "RSHIFT",
  [67] = "LCTRL",    [68] = "APPLICATION", [69] = "LALT",
  [70] = "SPACE",    [71] = "RALT",    [72] = "RCTRL",
  [73] = "UP",       [74] = "DOWN",    [75] = "LEFT",    [76] = "RIGHT",
  [77] = "INSERT",   [78] = "DELETE",  [79] = "HOME",    [80] = "END",
  [81] = "PAGEUP",   [82] = "PAGEDOWN",
};
#define KEY_NAMES_COUNT (sizeof(key_names) / sizeof(key_names[0]))

static bool has_uart, has_kbd;

static void drain_keys(void) {
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
      if (ev.keycode == 0) break;
      if ((size_t)ev.keycode < KEY_NAMES_COUNT) {
        printf("Got  (kbd): %s (%d) %s\n",
               key_names[ev.keycode], ev.keycode,
               ev.keydown ? "DOWN" : "UP");
      } else {
        printf("Got  (kbd): Unknown keycode (%d) %s\n",
               ev.keycode, ev.keydown ? "DOWN" : "UP");
      }
    }
  }
}

int main(const char *args) {
  ioe_init();

  printf("Try to press any key (uart or keyboard)...\n");
  has_uart = io_read(AM_UART_CONFIG).present;
  if (has_uart) {
    printf("UART present, waiting for input...\n");
  }
  has_kbd = io_read(AM_INPUT_CONFIG).present;
  if (has_kbd) {
    printf("Keyboard present, waiting for input...\n");
  }

  while (1) {
    drain_keys();
  }

  return 0;
}
