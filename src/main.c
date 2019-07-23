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
        runtest();
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
    Code const* code = program(tokens);
    generate(code);

    return 0;
}

// Output an error for user and exit.
void error_at(char const* loc, char const* msg)
{
    // Find head and tail of line which includes loc.
    char const* line = loc;
    while (input < line && line[-1] != '\n') {
        line--;
    }

    char const* end = loc;
    while (*end != '\n') {
        end++;
    }

    // Find where the found line is.
    size_t line_num = 1;
    for (char const* p = input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }

    // Print the line with filename and line number.
    size_t indent = fprintf(stderr, "%s:%zd: ", filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // Output the error and emphases the error location via "^".
    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ %s\n", msg);
    exit(1);
}

#ifndef SELFHOST_9MM
// Output log for me and exit.
void log_internal(char const* level, const char* file, const char* func, size_t line, char const* fmt, ...)
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
