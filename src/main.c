#include "9mm.h"

#ifdef SELFHOST_9MM
extern void* stderr;
#endif

static char const* input;
static char const* filename;

int main(int argc, char const* const* argv)
{
    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s [--test] [--str 'your program'] [FILEPATH]\n\n", argv[0]);
        printf("  --test run test\n");
        printf("  --str  input c codes as a string\n");
        return 1;
    }

    if (strncmp("--test", argv[1], 5) == 0) {
#ifndef SELFHOST_9MM
        runtest();
#endif
        return 0;
    }

    if (strncmp("--str", argv[1], 4) == 0) {
        // The given string is source code.
        input = argv[2];
    } else {
        // The given file contains source code.
        filename = argv[1];

        char* content = read_file(filename);
        input = preprocess(content, filename);
    }

    Vector const* tokens = tokenize(input);
    Node const* const* code = program(tokens);
    generate(code);

    return 0;
}

// Output an error for user and exit.
void error_at(char const* loc, char const* msg)
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

#ifndef SELFHOST_9MM
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
#else
void error_if_null(void* var)
{
    if (var == NULL) {
        error("NULL is given");
    }
}
#endif

char* read_file(char const* path)
{
    void* fp = fopen(path, "r");
    if (fp == NULL) {
        error("cannot open %s", path);
    }

    // FIXME: use SEEK_END
    if (fseek(fp, 0, 2) == -1) {
        error("%s: fseek", path);
    }

    // FIXME: use SEEK_SET
    size_t size = ftell(fp);
    if (fseek(fp, 0, 0) == -1) {
        error("%s: fseek", path);
    }

    char* buf = calloc(1, size + 2);
    fread(buf, size, 1, fp);

    // Enforce the content is terminated by "\n\0".
    if (size == 0 || buf[size - 1] != '\n') {
        buf[size++] = '\n';
    }
    buf[size] = '\0';

    fclose(fp);

    return buf;
}
