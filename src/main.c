#include "9mm.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char const* read_file(char const*);
static char const* input;
static char const* filename;

int main(int argc, char const* const* argv)
{
    if (argc < 2) {
        puts("9mm [--test] [--str 'your program'] [FILEPATH]");
        return 1;
    }

    if (strncmp("--test", argv[1], 5) == 0) {
        runtest();
        return 0;
    }

    if (strncmp("--str", argv[1], 4) == 0) {
        // The given string is source code.
        input = argv[2];
    } else {
        // The given file contains source code.
        filename = argv[1];
        input = read_file(filename);
    }

    Vector const* tokens = tokenize(input);
    Node const* const* code = program(tokens);

    puts(".intel_syntax noprefix");
    puts(".global main\n");

    Vector const* keys = str_label_map->keys;
    if (keys->len != 0) {
        // Define string literals.
        puts(".data");
        puts(".align 8");
        for (size_t i = keys->len; 0 < i; i--) {
            char const* str = str_label_map->keys->data[i - 1];
            char const* label = str_label_map->vals->data[i - 1];
            printf("%s:\n", label);
            printf("  .string %s\n", str);
        }
        putchar('\n');
    }

    // Allocate the global variable spaces.
    puts("# Global variables");
    puts(".bss");
    puts(".align 32");
    for (size_t i = 0; code[i]; i++) {
        if (code[i]->ty == ND_GVAR_NEW) {
            printf("%s:\n", code[i]->name);
            printf("  .zero %zd\n", code[i]->rtype->size);
        }
    }
    putchar('\n');
    puts(".text");

    // 先頭の関数から順にコード生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);
    }

    return 0;
}

// Output an error for user and exit.
_Noreturn void error_at(char const* loc, char const* msg)
{
    // locが含まれている行の開始地点と終了地点を取得
    char const* line = loc;
    while (input < line && line[-1] != '\n') {
        line--;
    }

    char const* end = loc;
    while (*end != '\n') {
        end++;
    }

    // 見つかった行が全体の何行目なのかを調べる
    size_t line_num = 1;
    for (char const* p = input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }

    // 見つかった行を、ファイル名と行番号と一緒に表示
    size_t indent = fprintf(stderr, "%s:%zd: ", filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // エラー箇所を"^"で指し示して、エラーメッセージを表示
    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ %s\n", msg);
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

// 指定されたファイルの内容を返す
static char const* read_file(char const* path)
{
    // ファイルを開く
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        error("cannot open %s: %s", path, strerror(errno));
    }

    // ファイルの長さを調べる
    if (fseek(fp, 0, SEEK_END) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
    }
    size_t size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
    }

    // ファイル内容を読み込む
    char* buf = calloc(1, size + 2);
    fread(buf, size, 1, fp);

    // ファイルが必ず"\n\0"で終わっているようにする
    if (size == 0 || buf[size - 1] != '\n') {
        buf[size++] = '\n';
    }
    buf[size] = '\0';

    fclose(fp);

    return buf;
}
