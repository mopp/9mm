#ifndef H_9MM
#define H_9MM


#include "container.h"
#include <stddef.h>
#include <stdint.h>

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
    TK_IDENT, // 識別子
    TK_NUM,   // 整数トークン
    TK_EOF,   // 入力の終わりを表すトークン
    TK_IF,    // if
    TK_ELSE,  // else
    TK_WHILE, // while
    TK_FOR,   // for
    TK_EQ,    // ==
    TK_NE,    // !=
    TK_LE,    // <=
    TK_GE,    // >=
};

typedef struct {
    int ty;            // トークンの型
    int val;           // tyがTK_NUMの場合、その数値
    char const* name;  // tyがTK_IDENTの場合、その名前
    char const* input; // トークン文字列（エラーメッセージ用）
} Token;

typedef struct Node {
    int ty;                  // 演算子、ND_NUMかND_LVAR
    struct Node* lhs;        // 左辺
    struct Node* rhs;        // 右辺
    int val;                 // tyがND_NUMの場合のみ使う
    uintptr_t offset;        // tyがND_LVARの場合のみ使う
    void* type_depend_value; // tyで変わる
} Node;

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

typedef struct {
    size_t count_vars;
    Map* var_offset_map; // variable name -> offset.
} Context;

typedef struct {
    char const* name;
    Context* context;
} NodeFunction;

typedef struct Type {
    enum { INT,
           PTR } ty;
    struct Type* ptr_to;
} Type;

// main.c
void error_at(char const*, char const*);
void error(char const*, ...);

// tokenize.c
Vector const* tokenize(char const*);

// parse.c
Node const* const* program(Vector const*);

// codegen.c
void gen(Node const*);

void runtest();


#endif /* end of include guard */
