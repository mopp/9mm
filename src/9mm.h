#ifndef H_9MM
#define H_9MM


#include "container.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    // 抽象構文木の型を表す値
    ND_NUM = 256, // 整数のノードの型
    ND_RETURN,
    ND_LVAR_NEW, // ローカル変数の宣言
    ND_LVAR,     // ローカル変数の参照
    ND_IF,       // if
    ND_WHILE,    // while
    ND_FOR,      // for
    ND_BLOCK,    // "{" stmt* "}"
    ND_CALL,     // 関数呼び出し
    ND_FUNCTION, // 関数定義
    ND_REF,      // reference
    ND_DEREF,    // dereference

    // トークンの型を表す値
    TK_RETURN,
    TK_IDENT,  // 識別子
    TK_NUM,    // 整数トークン
    TK_EOF,    // 入力の終わりを表すトークン
    TK_IF,     // if
    TK_ELSE,   // else
    TK_WHILE,  // while
    TK_FOR,    // for
    TK_EQ,     // ==
    TK_NE,     // !=
    TK_LE,     // <=
    TK_GE,     // >=
    TK_SIZEOF, // sizeof
};

typedef struct {
    int ty; // トークンの型
    union {
        int val;          // tyがTK_NUMの場合、その数値
        char const* name; // tyがTK_IDENTの場合、その名前
    };
    char const* input; // トークン文字列（エラーメッセージ用）
} Token;

typedef struct {
    size_t count_vars;
    Map* var_offset_map; // variable name -> offset.
    Map* var_type_map;   // variable name -> "Type".
} Context;

typedef struct {
    char const* name;
    Context* context;
} NodeFunction;

typedef struct {
    struct Node* condition;
    struct Node* body;
    struct Node* else_body; // NULLはelse節が無いことを意味する
} NodeIfElse;

typedef struct {
    struct Node* initializing;
    struct Node* condition;
    struct Node* updating;
    struct Node* body;
} NodeFor;

typedef struct {
    char const* name;
    Vector* arguments;
} NodeCall;

typedef struct Type {
    enum { INT,
           PTR } ty;
    struct Type* ptr_to;
    size_t size;
} Type;

typedef struct Node {
    int ty;            // Type of "Node"
    struct Node* lhs;  // Left-hand-side
    struct Node* rhs;  // Right-hand-size
    Type const* rtype; // Type of result of "expr" of "Node".
    union {
        int val;          // for "ND_NUM"
        char const* name; // for "ND_LVAR"
        NodeFunction* function;
        Vector* stmts;
        NodeIfElse* if_else;
        NodeFor* fors;
        NodeCall* call;
        void* tv;
    };
} Node;

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

#define debug(...) \
    __log("\033[1;34m[DEBUG]\033[0m", __VA_ARGS__);

// tokenize.c
Vector const* tokenize(char const*);

// parse.c
Node const* const* program(Vector const*);

// codegen.c
void gen(Node const*);

void runtest();


#endif /* end of include guard */
