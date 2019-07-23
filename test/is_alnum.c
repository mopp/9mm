#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_alnum(char c)
{
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}

int main(void)
{
    char* p = "a _b   c";

    printf("%d\n", is_alnum(*(p + 0)));
    printf("%d\n", is_alnum(*(p + 1)));
    printf("%d\n", is_alnum(*(p + 2)));
    printf("%d\n", is_alnum(*(p + 3)));
    printf("%d\n", is_alnum(*(p + 4)));

    return 0;
}
