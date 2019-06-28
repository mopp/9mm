#ifndef H_9MM
#define H_9MM

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
    int ty;      // トークンの型
    int val;     // tyがTK_NUMの場合、その数値
    char* name;  // tyがTK_IDENTの場合、その名前
    char* input; // トークン文字列（エラーメッセージ用）
} Token;

typedef struct {
    void** data;
    int capacity;
    int len;
} Vector;

typedef struct {
    Vector* keys;
    Vector* vals;
} Map;

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
    char* name;
    Vector* arguments;
} NodeCall;

typedef struct {
    char* name;
    size_t count_local_variables;
    Map* variable_name_map;
} NodeFunction;

typedef struct Type {
    enum { INT,
           PTR } ty;
    struct Type* ptr_to;
} Type;

// main.c
void error_at(char const*, char const*);
void error(char const*, ...);

// parse.c
void tokenize(char const*);
Node const* const* program();

// codegen.c
void gen(Node*);

// container.c
Vector* new_vector();
void vec_push(Vector*, void*);
Map* new_map();
void map_put(Map*, char*, void*);
void* map_get(Map*, char*);

void runtest();


#endif /* end of include guard */
