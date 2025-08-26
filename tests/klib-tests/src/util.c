#include <stdio.h>

#include <klibtest.h>

#define COLOR_RED   "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_RESET "\033[0m"

void print_result(const char *name, bool passed) {
    printf("%s: %s\n", name, passed ?
        COLOR_GREEN "PASSED" COLOR_RESET :
        COLOR_RED "FAILED" COLOR_RESET);
}
