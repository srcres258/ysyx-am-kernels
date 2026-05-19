#include <amtest.h>

#define FPS 30
#define N   32

static inline uint32_t pixel(uint8_t r, uint8_t g, uint8_t b) {
  return (r << 16) | (g << 8) | b;
}
static inline uint8_t R(uint32_t p) { return p >> 16; }
static inline uint8_t G(uint32_t p) { return p >> 8; }
static inline uint8_t B(uint32_t p) { return p; }

static uint32_t canvas[N][N];
static int used[N][N];

static uint32_t color_buf[32 * 32];

static inline void outl(uintptr_t addr, uint32_t data) { *(volatile uint32_t *)addr = data; }

void redraw() {
  int w = io_read(AM_GPU_CONFIG).width / N;
  int h = io_read(AM_GPU_CONFIG).height / N;
  int block_size = w * h;
  assert((uint32_t)block_size <= LENGTH(color_buf));

  int x, y, k;
  printf("Redrawing the canvas\n");
  for (y = 0; y < N; y ++) {
    printf("y = %d\n", y);
    for (x = 0; x < N; x ++) {
      for (k = 0; k < block_size; k ++) {
        color_buf[k] = canvas[y][x];
      }
      io_write(AM_GPU_FBDRAW, x * w, y * h, color_buf, w, h, false);
    }
  }
  io_write(AM_GPU_FBDRAW, 0, 0, NULL, 0, 0, true);
}

static uint32_t p(int tsc) {
  int b = tsc & 0xff;
  return pixel(b * 6, b * 7, b);
}

void update() {
  static int tsc = 0;
  static int dx[4] = {0, 1, 0, -1};
  static int dy[4] = {1, 0, -1, 0};

  tsc ++;
  
  printf("N=%d\n", N);

  for (int i = 0; i < N; i ++)
    for (int j = 0; j < N; j ++) {
      used[i][j] = 0;
    }

  printf("Beginning to fill the canvas\n");
  int init = tsc * 1;
  canvas[0][0] = p(init); used[0][0] = 1;
  int x = 0, y = 0, d = 0;
  for (int step = 1; step < N * N; step ++) {
    if (step % 256 == 0)
      printf("Step %d\n", step);
    for (int t = 0; t < 4; t ++) {
      int x1 = x + dx[d], y1 = y + dy[d];
      if (x1 >= 0 && x1 < N && y1 >= 0 && y1 < N && !used[x1][y1]) {
        x = x1; y = y1;
        used[x][y] = 1;
        canvas[x][y] = p(init + step / 2);
        break;
      }
      d = (d + 1) % 4;
    }
  }
  printf("Done filling the canvas\n");
}

void video_test() {
  printf("Video device test\n");

  unsigned long last = 0;
  unsigned long fps_last = 0;
  int fps = 0;

  while (1) {
    printf("Drawing\n");
    unsigned long upt = io_read(AM_TIMER_UPTIME).us / 1000;
    if (upt - last > 1000 / FPS) {
      printf("Updating\n");
      update();
      redraw();
      last = upt;
      fps ++;
    }
    if (upt - fps_last > 1000) {
      // display fps every 1s
      printf("%d: FPS = %d\n", upt, fps);
      fps_last = upt;
      fps = 0;
    }
    printf("Loop done\n");
  }
}
