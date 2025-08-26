#include <stdio.h>
#include <stdlib.h>

#include <klibtest.h>

/**
 * @brief 测试点函数入口
 */
static void (*entry)() = NULL;

#define CASE(id, entry_, ...) \
    case id: { \
        void entry_(); \
        entry = entry_; \
        __VA_ARGS__; \
        entry_(); \
        break; \
    }

static const char *tests[256] = {
    ['h'] = "hello",
    ['H'] = "display this help message",
    ['s'] = "string test",
};

static void print_usage(void) {
    int ch;

    printf("Usage: make run mainargs=*\n");
    for (ch = 0; ch < 256; ch++) {
        if (tests[ch]) {
            printf("  %c: %s\n", ch, tests[ch]);
        }
    }
}

int main(const char *args) {
    switch (args[0]) {
        CASE('h', hello);
        CASE('s', test_string);

        case 'H': default:
            print_usage();
    }

    return EXIT_SUCCESS;
}
