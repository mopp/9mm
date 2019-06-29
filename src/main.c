#include "9mm.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char const* input;

int main(int argc, char const* const* argv)
{
    if (argc != 2) {
        error("引数の個数が正しくありません");
        return 1;
    }

    if (strncmp("-test", argv[1], 5) == 0) {
        runtest();
        return 0;
    }

    // トークナイズする
    input = argv[1];
    Vector const* tokens = tokenize(input);
    Node const* const* code = program(tokens);

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");

    // 先頭の関数から順にコード生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);
    }

    return 0;
}

// Output an error for user and exit.
_Noreturn void error_at(char const* loc, char const* msg)
{
    int pos = loc - input;
    fprintf(stderr, "\e[1m%s\n%*s^ %s\n\e[0m", input, pos, "", msg);
    exit(1);
}

// Output log for me and exit.
void _log(char const* level, const char* file, const char* func, size_t line, char const* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s [%s:%s:%zd] ", level, file, func, line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}
