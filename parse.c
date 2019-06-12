#include "9mm.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static Node* new_node(int, Node*, Node*);
static Node* new_node_num(int);
static Node* stmt();
static Node* expr();
static Node* assign();
static Node* equality();
static Node* relational();
static Node* add();
static Node* mul();
static Node* unary();
static Node* term();
static int consume(int);
static int is_alnum(char);

// 入力プログラム
char* user_input;

// トークナイズした結果のトークン列
Vector* tokens = NULL;

// 現在読んでいるトークンの位置.
int pos;

// 式の集まり.
Node* code[100];

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

        // NOTE: "return "との比較では対応出来ないケースがあるか考える.
        if ((strncmp(p, "return", 6) == 0) && !is_alnum(p[6])) {
            vec_push_token(tokens, TK_RETURN, 0, p);
            i++;
            p += 6;
            continue;
        }

        if ('a' <= *p && *p <= 'z') {
            vec_push_token(tokens, TK_IDENT, 0, p);
            i++;
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

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>' || *p == ';' || *p == '=') {
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

// program    = stmt*
// stmt       = expr ";" | "return" expr ";"
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? term
// term       = num | ident | "(" expr ")
void program()
{
    int i = 0;
    Token** ts = (Token**)(tokens->data);
    while (ts[pos]->ty != TK_EOF) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

static Node* stmt()
{
    Node* node;

    if (consume(TK_RETURN)) {
        node = malloc(sizeof(Node));
        node->ty = ND_RETURN;
        node->lhs = expr();
    } else {
        node = expr();
    }

    if (!consume(';')) {
        Token** ts = (Token**)(tokens->data);
        error_at(ts[pos]->input, "';'ではないトークンです");
    }

    return node;
}

static Node* expr()
{
    return assign();
}

static Node* assign()
{
    Node* node = equality();
    if (consume('='))
        node = new_node('=', node, assign());
    return node;
}

static Node* equality()
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

static Node* relational()
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

static Node* add()
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

static Node* mul()
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

static Node* unary()
{
    if (consume('+'))
        return term();
    if (consume('-'))
        return new_node('-', new_node_num(0), term());
    return term();
}

static Node* term()
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
    if (ts[pos]->ty == TK_NUM) {
        return new_node_num(ts[pos++]->val);
    } else if (ts[pos]->ty == TK_IDENT) {
        // 変数名はaからzの位置文字のみ
        // 関数フレーム上でもアルファベットで固定の位置とする
        char varname = ts[pos++]->input[0];

        Node* node = malloc(sizeof(Node));
        node->ty = ND_LVAR;
        node->offset = (varname - 'a' + 1) * 8;
        return node;
    }

    error_at(ts[pos]->input,
             "数値でも開きカッコでもないトークンです");

    return NULL;
}

// 期待した型であれば1トークン読み進める
// 成功したら1が返る
static int consume(int ty)
{
    Token** ts = (Token**)(tokens->data);
    if (ts[pos]->ty != ty)
        return 0;
    pos++;
    return 1;
}

static Node* new_node(int ty, Node* lhs, Node* rhs)
{
    Node* node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node* new_node_num(int val)
{
    Node* node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

static int is_alnum(char c)
{
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}
