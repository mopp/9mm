#define _XOPEN_SOURCE 700

#include "9mm.h"
#include "container.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


static int is_alnum(char);


Vector const* tokenize(char const* p)
{
    Vector* tokens = new_vector();

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        Token* token = malloc(sizeof(Token));

        // NOTE: "return "との比較では対応出来ないケースがあるか考える.
        if ((strncmp(p, "return", 6) == 0) && !is_alnum(p[6])) {
            token->ty = TK_RETURN;
            token->input = p;
            vec_push(tokens, token);
            p += 6;
            continue;
        }

        if ((strncmp(p, "if", 2) == 0) && !is_alnum(p[2])) {
            token->ty = TK_IF;
            token->input = p;
            vec_push(tokens, token);
            p += 2;
            continue;
        }

        if ((strncmp(p, "else", 4) == 0) && !is_alnum(p[4])) {
            token->ty = TK_ELSE;
            token->input = p;
            vec_push(tokens, token);
            p += 4;
            continue;
        }

        if ((strncmp(p, "while", 5) == 0) && !is_alnum(p[5])) {
            token->ty = TK_WHILE;
            token->input = p;
            vec_push(tokens, token);
            p += 5;
            continue;
        }

        if ((strncmp(p, "for", 3) == 0) && !is_alnum(p[3])) {
            token->ty = TK_FOR;
            token->input = p;
            vec_push(tokens, token);
            p += 3;
            continue;
        }

        if (strncmp(p, "==", 2) == 0) {
            token->ty = TK_EQ;
            token->input = p;
            vec_push(tokens, token);
            p += 2;
            continue;
        }

        if (strncmp(p, "!=", 2) == 0) {
            token->ty = TK_NE;
            token->input = p;
            vec_push(tokens, token);
            p += 2;
            continue;
        }

        if (strncmp(p, "<=", 2) == 0) {
            token->ty = TK_LE;
            token->input = p;
            vec_push(tokens, token);
            p += 2;
            continue;
        }

        if (strncmp(p, ">=", 2) == 0) {
            token->ty = TK_GE;
            token->input = p;
            vec_push(tokens, token);
            p += 2;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>' || *p == ';' || *p == '=' || *p == '{' || *p == '}' || *p == ',' || *p == '&') {
            token->ty = *p;
            token->input = p;
            vec_push(tokens, token);
            p++;
            continue;
        }

        if (isdigit(*p)) {
            token->ty = TK_NUM;
            token->input = p;
            token->val = strtol(p, (char**)&p, 10);
            vec_push(tokens, token);
            continue;
        }

        // find the variable name.
        char const* name = p;
        while (is_alnum(*p)) {
            ++p;
        }

        if (name != p) {
            size_t n = p - name;
            token->ty = TK_IDENT;
            token->name = strndup(name, n);
            token->input = name;
            vec_push(tokens, token);
            continue;
        }

        free(token);

        error_at(p, "トークナイズできません");
    }

    Token* token = malloc(sizeof(Token));
    token->ty = TK_EOF;
    token->input = p;
    vec_push(tokens, token);

    return tokens;
}

static int is_alnum(char c)
{
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}