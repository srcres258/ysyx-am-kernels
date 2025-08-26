#include <stdio.h>

void hello(void) {
#ifdef __ISA__
    printf("Hello, world! ISA is: " __ISA__ "\n");
#else
    printf("Hello, world! ISA is unknown.\n");
#endif
}
