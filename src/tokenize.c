#include "9mm.h"

#ifndef SELFHOST_9MM
static int is_eq(char const*, char const*);
static Token* new_token(int, char const*);
static char const* skip(char const*);
static int is_alnum(char);
#endif

Vector const* tokenize(char const* p)
{
    Vector* tokens = new_vector();

    while (*p) {
        p = skip(p);

        Token* token = NULL;
        if (*p == '\0') {
            break;
        } else if (is_eq(p, "extern")) {
            token = new_token(TK_EXTERN, p);
            p += 6;
        } else if (is_eq(p, "typedef")) {
            token = new_token(TK_TYPEDEF, p);
            p += 7;
        } else if (is_eq(p, "enum ")) {
            token = new_token(TK_ENUM, p);
            p += 4;
        } else if (is_eq(p, "break;")) {
            token = new_token(TK_BREAK, p);
            p += 5;
        } else if (is_eq(p, "+=")) {
            token = new_token(TK_ADD_ASIGN, p);
            p += 2;
        } else if (is_eq(p, "-=")) {
            token = new_token(TK_SUB_ASIGN, p);
            p += 2;
        } else if (is_eq(p, "*=")) {
            token = new_token(TK_MUL_ASIGN, p);
            p += 2;
        } else if (is_eq(p, "/=")) {
            token = new_token(TK_DIV_ASIGN, p);
            p += 2;
        } else if (is_eq(p, "->")) {
            token = new_token(TK_ARROW, p);
            p += 2;
        } else if (is_eq(p, "struct")) {
            token = new_token(TK_STRUCT, p);
            p += 6;
        } else if (is_eq(p, "||")) {
            token = new_token(TK_OR, p);
            p += 2;
        } else if (is_eq(p, "&&")) {
            token = new_token(TK_AND, p);
            p += 2;
        } else if (is_eq(p, "++")) {
            token = new_token(TK_INCL, p);
            p += 2;
        } else if (is_eq(p, "--")) {
            token = new_token(TK_DECL, p);
            p += 2;
        } else if (*p == '"') {
            // Read string literal.
            char const* str_begin = p++;
            while (*p != '"') {
                ++p;
            }
            token = new_token(TK_STR, str_begin);
            token->name = strndup(str_begin, p - str_begin + 1);
            ++p;
        } else if (is_eq(p, "sizeof")) {
            token = new_token(TK_SIZEOF, p);
            p += 6;
        } else if (is_eq(p, "return") && !is_alnum(p[6])) {
            token = new_token(TK_RETURN, p);
            p += 6;
        } else if (is_eq(p, "if") && !is_alnum(p[2])) {
            token = new_token(TK_IF, p);
            p += 2;
        } else if (is_eq(p, "else") && !is_alnum(p[4])) {
            token = new_token(TK_ELSE, p);
            p += 4;
        } else if (is_eq(p, "while") && !is_alnum(p[5])) {
            token = new_token(TK_WHILE, p);
            p += 5;
        } else if (is_eq(p, "for") && !is_alnum(p[3])) {
            token = new_token(TK_FOR, p);
            p += 3;
        } else if (is_eq(p, "==")) {
            token = new_token(TK_EQ, p);
            p += 2;
        } else if (is_eq(p, "!=")) {
            token = new_token(TK_NE, p);
            p += 2;
        } else if (is_eq(p, "<=")) {
            token = new_token(TK_LE, p);
            p += 2;
        } else if (is_eq(p, ">=")) {
            token = new_token(TK_GE, p);
            p += 2;
        } else if (*p == '+' || *p == '-' ||
                   *p == '*' || *p == '/' ||
                   *p == '(' || *p == ')' ||
                   *p == '<' || *p == '>' ||
                   *p == ';' || *p == '=' ||
                   *p == '{' || *p == '}' ||
                   *p == ',' || *p == '&' ||
                   *p == '[' || *p == ']' ||
                   *p == '.' || *p == '!') {
            token = new_token(*p, p);
            ++p;
        } else if (isdigit(*p)) {
            token = new_token(TK_NUM, p);
            token->val = strtol(p, (char**)&p, 10);
        } else if (*p == 39) {
            // 39 == '\''
            token = new_token(TK_NUM, p);
            ++p;
            if (*p == 92) {
                // 92 == '\'
                ++p;
                if (*p == 'n') {
                    token->val = 10;
                } else if (*p == '0') {
                    token->val = 0;
                }
            } else {
                token->val = *p;
            }
            p += 2;
        } else {
            // Find the variable name.
            char const* name = p;
            while (is_alnum(*p)) {
                ++p;
            }

            if (name != p) {
                size_t n = p - name;
                token = new_token(TK_IDENT, name);
                token->name = strndup(name, n);
            }
        }

        if (token == NULL) {
            error_at(p, "It cannot tokenize");
        }

        vec_push(tokens, token);
    }

    vec_push(tokens, new_token(TK_EOF, p));

    return tokens;
}

static int is_eq(char const* p, char const* q)
{
    return strncmp(p, q, strlen(q)) == 0;
}

static Token* new_token(int ty, char const* p)
{
    Token* token = malloc(sizeof(Token));
    token->ty = ty;
    token->input = p;

    return token;
}

static char const* skip(char const* p)
{
    while (*p) {
        if (isspace(*p)) {
            // Skip space.
            ++p;
        } else if (strncmp(p, "//", 2) == 0) {
            // Skip line comment.
            p += 2;
            while (*p != '\n') {
                ++p;
            }
        } else if (strncmp(p, "/*", 2) == 0) {
            // Skip block comment.
            char* q = strstr(p + 2, "*/");
            if (!q) {
                error_at(p, "The comment is NOT closed.");
            }
            p = q + 2;
        } else {
            break;
        }
    }

    return p;
}

static int is_alnum(char c)
{
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}
