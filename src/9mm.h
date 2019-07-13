#ifndef H_9MM
#define H_9MM

#define _XOPEN_SOURCE 700

#include "container.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    // 抽象構文木の型を表す値
    ND_NUM = 256, // 整数のノードの型
    ND_RETURN,
    ND_LVAR_NEW,  // Declare local variable.
    ND_LVAR,      // Reference local variable.
    ND_GVAR_NEW,  // Declare global variable.
    ND_GVAR,      // Reference global variable.
    ND_IF,        // if
    ND_WHILE,     // while
    ND_FOR,       // for
    ND_BLOCK,     // "{" stmt* "}"
    ND_CALL,      // 関数呼び出し
    ND_FUNCTION,  // 関数定義
    ND_REF,       // reference
    ND_DEREF,     // dereference
    ND_STR,       // String literal
    ND_INCL_POST, // i++
    ND_DECL_POST, // i--
    ND_AND,       // &&
    ND_OR,        // ||
    ND_DOT_REF,   // obj.x
    ND_ARROW_REF, // obj->x
    ND_INIT,      // int x = 3
    ND_BREAK,     // break

    // トークンの型を表す値
    TK_RETURN,
    TK_IDENT,     // 識別子
    TK_NUM,       // 整数トークン
    TK_EOF,       // 入力の終わりを表すトークン
    TK_IF,        // if
    TK_ELSE,      // else
    TK_WHILE,     // while
    TK_FOR,       // for
    TK_EQ,        // ==
    TK_NE,        // !=
    TK_LE,        // <=
    TK_GE,        // >=
    TK_SIZEOF,    // sizeof
    TK_STR,       // String literal
    TK_INCL,      // ++
    TK_DECL,      // --
    TK_AND,       // &&
    TK_OR,        // ||
    TK_STRUCT,    // struct
    TK_ARROW,     // obj->x
    TK_ADD_ASIGN, // +=
    TK_SUB_ASIGN, // -=
    TK_MUL_ASIGN, // *=
    TK_DIV_ASIGN, // /=
    TK_BREAK,     // break
    TK_ENUM,      // enum
    TK_TYPEDEF    // typedef
};

struct token {
    int ty; // トークンの型
    union {
        int val;          // tyがTK_NUMの場合、その数値
        char const* name; // tyがTK_IDENTの場合、その名前
    };
    char const* input; // トークン文字列（エラーメッセージ用）
};
typedef struct token Token;

struct context {
    size_t count_vars;
    size_t current_offset;   // The offset of the current "rbp" register.
    Map* var_offset_map;     // variable name -> offset.
    Map* var_type_map;       // variable name -> "Type".
    char const* break_label; // Label for break in current loop
};
typedef struct context Context;

struct node_function {
    char const* name;
    Vector* args;
    Context* context;
};
typedef struct node_function NodeFunction;

struct node_if_else {
    struct node* condition;
    struct node* body;
    struct node* else_body; // NULLはelse節が無いことを意味する
};
typedef struct node_if_else NodeIfElse;

struct node_for {
    struct node* initializing;
    struct node* condition;
    struct node* updating;
    struct node* body;
    char const* break_label;
};
typedef struct node_for NodeFor;

struct node_call {
    char const* name;
    Vector* arguments;
};
typedef struct node_call NodeCall;

struct user_type {
    char const* name;
    size_t size;
    Map* member_offset_map; // name -> its offset
    Map* member_type_map;   // name -> its type
};
typedef struct user_type UserType;

struct type {
    enum { CHAR,
           INT,
           PTR,
           ARRAY,
           USER,
           VOID } ty;
    struct type const* ptr_to;
    size_t size;
    UserType* user_type; // Valid if ty is USER
};
typedef struct type Type;

struct node {
    int ty;            // Type of "Node"
    struct node* lhs;  // Left-hand-side
    struct node* rhs;  // Right-hand-size
    Type const* rtype; // Type of result of "expr" of "Node".
    union {
        int val;              // for "ND_NUM"
        char const* name;     // for "ND_LVAR"
        size_t member_offset; // for "ND_DOT_REF"
        NodeFunction* function;
        Vector* stmts;
        NodeIfElse* if_else;
        NodeFor* fors;
        NodeCall* call;
        char const* label; // for "ND_STR"
        char const* break_label;
        void* tv;
    };
};
typedef struct node Node;

// main.c
_Noreturn void error_at(char const*, char const*);
void _log(char const*, const char*, const char*, size_t, char const*, ...);

#define __log(level, ...) \
    _log(level, __FILE__, __func__, __LINE__, __VA_ARGS__);

#define error(...)                                      \
    {                                                   \
        __log("\033[1;31m[ERROR]\033[0m", __VA_ARGS__); \
        exit(1);                                        \
    }

#define error_if_null(var)          \
    {                               \
        if (var == NULL) {          \
            error(#var " is NULL"); \
        }                           \
    }

#define debug(...) \
    __log("\033[1;34m[DEBUG]\033[0m", __VA_ARGS__);

// tokenize.c
Vector const* tokenize(char const*);

// parse.c
Node const* const* program(Vector const*);
extern Map* str_label_map;
extern Map* user_types;

// codegen.c
void generate(Node const* const*);

void runtest();


#endif /* end of include guard */
