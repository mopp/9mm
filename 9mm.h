#ifndef H_9MM
#define H_9MM


enum {
    // 抽象構文木の型を表す値
    ND_NUM = 256, // 整数のノードの型
    ND_LVAR,

    // トークンの型を表す値
    TK_IDENT, // 識別子
    TK_NUM,   // 整数トークン
    TK_EOF,   // 入力の終わりを表すトークン
    TK_EQ,    // ==
    TK_NE,    // !=
    TK_LE,    // <=
    TK_GE,    // >=
};

typedef struct Node {
    int ty;           // 演算子、ND_NUMかND_LVAR
    struct Node* lhs; // 左辺
    struct Node* rhs; // 右辺
    int val;          // tyがND_NUMの場合のみ使う
    int offset;       // tyがND_LVARの場合のみ使う
} Node;

// トークンの型
typedef struct {
    int ty;      // トークンの型
    int val;     // tyがTK_NUMの場合、その数値
    char* input; // トークン文字列（エラーメッセージ用）
} Token;

typedef struct {
    void** data;
    int capacity;
    int len;
} Vector;


// main.c
void error_at(char*, char*);
void error(char*, ...);

// parse.c
extern char* user_input;
extern Vector* tokens;
extern int pos;
extern Node* code[100];
void tokenize();
void program();

// codegen.c
void gen(Node*);

// container.c
Vector* new_vector();
void vec_push(Vector*, void*);
void vec_push_token(Vector*, int, int, char*);
void runtest();


#endif /* end of include guard */
