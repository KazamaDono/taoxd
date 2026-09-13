#include <stdio.h>

int main(void)
{
    puts("first call: resolved lazily through the PLT");
    puts("second call: the GOT slot is already patched");
    printf("and printf resolves on its own first call: %d\n", 1234);
    return 0;
}
