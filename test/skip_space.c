#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static char* skip_space(char* p)
{
    while (*p) {
        if (*p == ' ') {
            ++p;
        } else {
            break;
        }
    }

    return p;
}

int main(void) {
    char* p = "a   b   c";

    p = skip_space(p);
    printf("%c\n", *p);
    p = skip_space(p + 1);
    printf("%c\n", *p);
    p = skip_space(p + 1);
    printf("%c\n", *p);
    p = skip_space(p);
    printf("%c\n", *p);

    return 0;
}
