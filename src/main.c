#include "9mm.h"

static char* read_file(char const*);
static char const* preprocess(char*, char const*);
static char* load_headers(char*, char const*);
static void expand_macros(char*);
static void truncate(char*, char const*);
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
static char* read_file(char const* path)
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

static char const* preprocess(char* content, char const* filepath)
{
    char* dir_path = NULL;
    char* p = strrchr(filepath, '/');
    if (p == NULL) {
        dir_path = "./";
    } else {
        dir_path = strndup(filepath, p - filepath);
    }

    content = load_headers(content, dir_path);

    if (p != NULL) {
        free(dir_path);
    }

    expand_macros(content);

    return content;
}

static char* load_headers(char* head, char const* dir_path)
{
    char* code_head = head;

    while (*head) {
        if (strncmp("#include <", head, 10) == 0) {
            // FIXME: Load system header.
            truncate(head, strchr(head, '\n'));
        } else if (strncmp("#include", head, 8) == 0) {
            // Extract target filename.
            char const* filename_head = head + 10;
            char const* filename_tail = strchr(filename_head, '"');
            size_t filename_size = filename_tail - filename_head;

            // Concat directory and filename.
            char* filepath = malloc(sizeof(char) * (strlen(dir_path) + filename_size + 1 + 1));
            *filepath = 0;
            strncat(filepath, dir_path, strlen(dir_path));
            strncat(filepath, "/", 2);
            strncat(filepath, filename_head, filename_size);

            char const* content = read_file(filepath);
            free(filepath);

            // Allocate space to store them enough.
            char* prog = malloc(sizeof(char) * (strlen(code_head) + strlen(content)));
            *prog = 0;

            // Load lines before current header.
            strncat(prog, code_head, head - code_head);
            // Load the header file.
            strncat(prog, content, strlen(content));
            // Load lines after the header.
            ++filename_tail;
            strncat(prog, filename_tail, strlen(filename_tail));

            free((void*)content);
            free((void*)code_head);

            code_head = prog;
            head = code_head;
        } else {
            head = strchr(head, '\n') + 1;
        }
    }

    return code_head;
}

// FIXME: Support nested macro and multiline macro.
static void expand_macros(char* head)
{
    char* stored_head = head;
    Map* macros = new_map();

    while (*head) {
        if (strncmp("#define ", head, 8) == 0) {
            char const* define_head = head + 8;
            char const* define_tail = strchr(define_head, '\n');

            char* define_ident = strndup(define_head, define_tail - define_head);
            map_put(macros, define_ident, define_ident);
            free(define_ident);

            // Remove the macro line.
            truncate(head, define_tail + 1);
        } else if (strncmp("#ifndef ", head, 8) == 0) {
            char const* ifndef_head = head + 8;
            char const* ifndef_tail = strchr(ifndef_head, '\n');

            char* ifndef_ident = strndup(ifndef_head, ifndef_tail - ifndef_head);
            int is_defined = map_get(macros, ifndef_ident) != NULL;
            free(ifndef_ident);

            // Remove #ifndef line.
            truncate(head, ifndef_tail + 1);

            char* else_head = strstr(head, "#else\n");
            char* else_tail = else_head + 5;
            char* endif_head = strstr(head, "#endif\n");
            char* endif_tail = endif_head + 8;
            int has_else = else_head != NULL && else_head < endif_head;

            if (is_defined) {
                // Keep the lines between #ifndef/#else and #endif.
                if (has_else) {
                    truncate(endif_head, endif_tail);
                    truncate(head, else_tail);
                } else {
                    truncate(head, endif_tail);
                }
            } else {
                // Keep the lines between #ifndef and #else.
                if (has_else) {
                    truncate(else_head, endif_tail);
                } else {
                    truncate(head, endif_tail);
                }
            }
        } else if (strncmp("#ifdef ", head, 7) == 0) {
            char const* ifdef_head = head + 7;
            char const* ifdef_tail = strchr(ifdef_head, '\n');

            char* ifdef_ident = strndup(ifdef_head, ifdef_tail - ifdef_head);
            int is_defined = map_get(macros, ifdef_ident) != NULL;
            free(ifdef_ident);

            // Remove #ifndef line.
            truncate(head, ifdef_tail + 1);

            char* else_head = strstr(head, "#else");
            char* else_tail = strchr(else_head, '\n') + 1;
            char* endif_head = strstr(head, "#endif");
            char* endif_tail = strchr(endif_head, '\n') + 1;
            int has_else = else_head != NULL && else_head < endif_head;

            if (!is_defined) {
                // Keep the lines between #ifdef/#else and #endif.
                if (has_else) {
                    truncate(endif_head, endif_tail);
                    truncate(head, else_tail);
                } else {
                    truncate(head, endif_tail);
                }
            } else {
                // Keep the lines between #ifdef and #else.
                if (has_else) {
                    truncate(else_head, endif_tail);
                } else {
                    truncate(head, endif_tail);
                }
            }
        } else {
            head = strchr(head, '\n') + 1;
        }
    }

    head = stored_head;

    // FIXME: read argument of "#define" and replace them here.
    while (*head) {
        char* null_head = strstr(head, "NULL");
        if (null_head != NULL && *(null_head - 1) != '"') {
            *null_head = '0';
            truncate(null_head + 1, null_head + 4);
        } else {
            head = strchr(head, '\n') + 1;
        }
    }
}

static void truncate(char* p, char const* tail)
{
    while (*tail) {
        *p = *tail;
        ++p;
        ++tail;
    }
    *p = '\0';
}
