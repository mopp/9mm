#include "9mm.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
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
    user_input = argv[1];
    tokenize();
    program();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");

    // 先頭の関数から順にコード生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);
    }

    return 0;
}

// エラー箇所を報告するための関数
void error_at(char const* loc, char const* msg)
{
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ %s\n", msg);
    exit(1);
}

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char const* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}
