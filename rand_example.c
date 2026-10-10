#include <stdio.h>
#include <stdlib.h>

int main(void) {
    /* A fixed seed makes repeated runs reproducible with the same libc. */
    srand(42);

    for (int i = 0; i < 10; ++i) {
        printf("%d\n", rand());
    }

    return 0;
}
