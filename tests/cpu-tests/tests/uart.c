int main(void) {
    char *uart_addr = (char *) 0x10000000;

    char str[] = "Hello, world! 123";
    int i;
    for (i = 0; i < 10; i++) {
        *uart_addr = str[i];
    }

    return 0;
}
