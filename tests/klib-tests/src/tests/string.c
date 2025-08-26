#include <stdio.h>

#include <klibtest.h>

void test_string(void) {
    // strlen 测试
    print_result("strlen-empty", strlen("") == 0);
    print_result("strlen-normal", strlen("hello") == 5);
    print_result("strlen-letters", strlen("abcdefghijklmnopqrstuvwxyz") == 26);

    // strcpy 测试
    char buf1[10];
    strcpy(buf1, "abc");
    print_result("strcpy-normal", strcmp(buf1, "abc") == 0);
    strcpy(buf1, "");
    print_result("strcpy-empty", strcmp(buf1, "") == 0);

    // strncpy 测试
    char buf2[10];
    strncpy(buf2, "abcdef", 3);
    print_result("strncpy-partial", buf2[0] == 'a' && buf2[1] == 'b' && buf2[2] == 'c' && buf2[3] == '\0');
    strncpy(buf2, "abc", 5);
    print_result("strncpy-pad", buf2[3] == '\0' && buf2[4] == '\0');
    strncpy(buf2, "", 2);
    print_result("strncpy-empty", buf2[0] == '\0' && buf2[1] == '\0');

    // strcat 测试
    char buf3[20] = "hello";
    strcat(buf3, " world");
    print_result("strcat-normal", strcmp(buf3, "hello world") == 0);
    buf3[0] = '\0';
    strcat(buf3, "abc");
    print_result("strcat-empty-dst", strcmp(buf3, "abc") == 0);

    // strcmp 测试
    print_result("strcmp-equal", strcmp("abc", "abc") == 0);
    print_result("strcmp-less", strcmp("abc", "abd") < 0);
    print_result("strcmp-greater", strcmp("abd", "abc") > 0);
    print_result("strcmp-empty", strcmp("", "") == 0);

    // strncmp 测试
    print_result("strncmp-equal", strncmp("abcdef", "abcxyz", 3) == 0);
    print_result("strncmp-less", strncmp("abc", "abd", 3) < 0);
    print_result("strncmp-greater", strncmp("abd", "abc", 3) > 0);
    print_result("strncmp-zero", strncmp("abc", "abd", 0) == 0);

    // memset 测试
    char buf4[5];
    memset(buf4, 'x', 5);
    print_result("memset-all", buf4[0] == 'x' && buf4[1] == 'x' && buf4[2] == 'x' && buf4[3] == 'x' && buf4[4] == 'x');
    memset(buf4, 0, 5);
    print_result("memset-zero", buf4[0] == 0 && buf4[1] == 0 && buf4[2] == 0 && buf4[3] == 0 && buf4[4] == 0);

    // memmove 测试
    char buf5[10] = "abcdef";
    memmove(buf5 + 2, buf5, 4); // 局部重叠
    print_result("memmove-overlap", buf5[2] == 'a' && buf5[3] == 'b' && buf5[4] == 'c' && buf5[5] == 'd');
    char buf6[10] = "abcdef";
    memmove(buf6, buf6 + 2, 4); // 局部重叠
    print_result("memmove-overlap-rev", buf6[0] == 'c' && buf6[1] == 'd' && buf6[2] == 'e' && buf6[3] == 'f');
    char buf7[10] = "abcdef";
    memmove(buf7, buf7, 6);     // 完全重叠
    print_result("memmove-same", strcmp(buf7, "abcdef") == 0);

    // memcpy 测试
    char buf8[10] = "123456";
    char buf9[10];
    memcpy(buf9, buf8, 7);
    print_result("memcpy-normal", strcmp(buf9, "123456") == 0);

    // memcmp 测试
    print_result("memcmp-equal", memcmp("abc", "abc", 3) == 0);
    print_result("memcmp-less", memcmp("abc", "abd", 3) < 0);
    print_result("memcmp-greater", memcmp("abd", "abc", 3) > 0);
    print_result("memcmp-zero", memcmp("abc", "abd", 0) == 0);

    printf("测试完成。\n");
}
