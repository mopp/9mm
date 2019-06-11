#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの型を表す値
enum {
    ND_NUM = 256, // 整数のノードの型
    TK_NUM,       // 整数トークン
    TK_EOF,       // 入力の終わりを表すトークン
    TK_EQ,        // ==
    TK_NE,        // !=
    TK_LE,        // <=
    TK_GE,        // >=
};

typedef struct Node {
    int ty;           // 演算子かND_NUM
    struct Node* lhs; // 左辺
    struct Node* rhs; // 右辺
    int val;          // tyがND_NUMの場合のみ使う
} Node;

// トークンの型
typedef struct {
    int ty;      // トークンの型
    int val;     // tyがTK_NUMの場合、その数値
    char* input; // トークン文字列（エラーメッセージ用）
} Token;

// 入力プログラム
char* user_input;

typedef struct {
    void** data;
    int capacity;
    int len;
} Vector;

// トークナイズした結果のトークン列
Vector* tokens = NULL;

// 現在読んでいるトークンの位置.
int pos;

Node* relational();
Node* add();
Node* mul();
Node* unary();
Node* term();
void error(char* fmt, ...);

Vector* new_vector()
{
    Vector* vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void*) * 16);
    vec->capacity = 16;
    vec->len = 0;
    return vec;
}

void vec_push(Vector* vec, void* elem)
{
    if (vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void*) * vec->capacity);
    }
    vec->data[vec->len++] = elem;
}

void vec_push_token(Vector* tokens, int ty, int val, char* input) {
    Token* t = malloc(sizeof(Token));
    t->ty = ty;
    t->val = val;
    t->input = input;
    vec_push(tokens, t);
}

// エラー箇所を報告するための関数
void error_at(char* loc, char* msg)
{
    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ %s\n", msg);
    exit(1);
}

// user_inputが指している文字列を
// トークンに分割してtokensに保存する
void tokenize()
{
    char* p = user_input;

    int i = 0;
    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (strncmp(p, "==", 2) == 0) {
            vec_push_token(tokens, TK_EQ, 0, p);
            i++;
            p += 2;
            continue;
        }

        if (strncmp(p, "!=", 2) == 0) {
            vec_push_token(tokens, TK_NE, 0, p);
            i++;
            p += 2;
            continue;
        }

        if (strncmp(p, "<=", 2) == 0) {
            vec_push_token(tokens, TK_LE, 0, p);
            i++;
            p += 2;
            continue;
        }

        if (strncmp(p, ">=", 2) == 0) {
            vec_push_token(tokens, TK_GE, 0, p);
            i++;
            p += 2;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>') {
            vec_push_token(tokens, *p, 0, p);
            i++;
            p++;
            continue;
        }

        if (isdigit(*p)) {
            vec_push_token(tokens, TK_NUM, strtol(p, &p, 10), p - 1);
            i++;
            continue;
        }

        error_at(p, "トークナイズできません");
    }


    vec_push_token(tokens, TK_EOF, 0, p);
}

Node* new_node(int ty, Node* lhs, Node* rhs)
{
    Node* node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node* new_node_num(int val)
{
    Node* node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

// 期待した型であれば1トークン読み進める
// 成功したら1が返る
int consume(int ty)
{
    Token** ts = (Token**)(tokens->data);
    if (ts[pos]->ty != ty)
        return 0;
    pos++;
    return 1;
}

// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? term
// term       = num | "(" expr ")
Node* expr()
{
    Node* node = relational();

    for (;;) {
        if (consume(TK_EQ))
            node = new_node(TK_EQ, node, relational());
        else if (consume(TK_NE))
            node = new_node(TK_NE, node, relational());
        else
            return node;
    }
}

Node* relational()
{
    Node* node = add();

    for (;;) {
        if (consume('<'))
            node = new_node('<', node, add());
        else if (consume(TK_LE))
            node = new_node(TK_LE, node, add());
        else if (consume('>'))
            node = new_node('<', add(), node);
        else if (consume(TK_GE))
            node = new_node(TK_LE, add(), node);
        else
            return node;
    }
}

Node* add()
{
    Node* node = mul();

    for (;;) {
        if (consume('+'))
            node = new_node('+', node, mul());
        else if (consume('-'))
            node = new_node('-', node, mul());
        else
            return node;
    }
}

Node* mul()
{
    Node* node = unary();

    for (;;) {
        if (consume('*'))
            node = new_node('*', node, term());
        else if (consume('/'))
            node = new_node('/', node, term());
        else
            return node;
    }
}

Node* unary()
{
    if (consume('+'))
        return term();
    if (consume('-'))
        return new_node('-', new_node_num(0), term());
    return term();
}

Node* term()
{
    Token** ts = (Token**)(tokens->data);

    // 次のトークンが'('なら、"(" expr ")"のはず
    if (consume('(')) {
        Node* node = expr();
        if (!consume(')'))
            error_at(ts[pos]->input,
                     "開きカッコに対応する閉じカッコがありません");
        return node;
    }

    // そうでなければ数値のはず
    if (ts[pos]->ty == TK_NUM)
        return new_node_num(ts[pos++]->val);

    error_at(ts[pos]->input,
             "数値でも開きカッコでもないトークンです");

    return NULL;
}

void gen(Node* node)
{
    if (node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->ty) {
        case TK_EQ:
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case TK_NE:
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case '<':
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case TK_LE:
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
        case TK_GE:
            error("Bug, parser: TK_GE exists\n");
            break;
        case '>':
            error("Bug, parser: > exists\n");
            break;
        case '+':
            printf("  add rax, rdi\n");
            break;
        case '-':
            printf("  sub rax, rdi\n");
            break;
        case '*':
            // rax * rdi -> rdxに上位、raxに下位
            printf("  imul rdi\n");
            break;
        case '/':
            // cqo命令でraxの値を128に拡張してrdxとraxにセットする
            printf("  cqo\n");
            printf("  idiv rdi\n");
    }

    printf("  push rax\n");
}

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void expect(int line, int expected, int actual)
{
    if (expected == actual)
        return;
    fprintf(stderr, "%d: %d expected, but got %d\n",
            line, expected, actual);
    exit(1);
}

void runtest()
{
    Vector* vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for (uintptr_t i = 0; i < 100; i++)
        vec_push(vec, (void*)i);

    expect(__LINE__, 100, vec->len);
    expect(__LINE__, 0, (uintptr_t)vec->data[0]);
    expect(__LINE__, 50, (uintptr_t)vec->data[50]);
    expect(__LINE__, 99, (uintptr_t)vec->data[99]);

    printf("OK\n");
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        error("引数の個数が正しくありません");
        return 1;
    }

    if (strncmp("-test", argv[1], 5) == 0) {
        runtest();
        return 0;
    }

    tokens = new_vector();

    // トークナイズする
    user_input = argv[1];
    tokenize();
    Node* node = expr();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 抽象構文木を下りながらコード生成
    gen(node);

    printf("  pop rax\n");
    printf("  ret\n");

    // To suppress warning.
    free(node);

    return 0;
}
