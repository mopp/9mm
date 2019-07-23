#include "9mm.h"

#ifndef SELFHOST_9MM
static char* load_headers(char*, char const*);
static void expand_macros(char*);
static void truncate(char*, char const*);
#endif

char const* preprocess(char* content, char const* filepath)
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
    map_put(macros, "SELFHOST_9MM", "SELFHOST_9MM");

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
            char* endif_tail = endif_head + 7;
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

            // Remove #ifdef line.
            truncate(head, ifdef_tail + 1);

            char* else_head = strstr(head, "#else\n");
            char* else_tail = else_head + 5;
            char* endif_head = strstr(head, "#endif\n");
            char* endif_tail = endif_head + 7;
            int has_else = else_head != NULL && else_head < endif_head;

            if (is_defined) {
                // Keep the lines between #ifdef and #else.
                if (has_else) {
                    truncate(else_head, endif_tail);
                } else {
                    truncate(endif_head, endif_tail);
                }
            } else {
                // Keep the lines between #ifdef/#else and #endif.
                if (has_else) {
                    truncate(endif_head, endif_tail);
                    truncate(head, else_tail);
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
