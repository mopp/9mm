#define _XOPEN_SOURCE 700

#include "9mm.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static Node* new_node(int, Node*, Node*);
static Node* new_node_num(int);
static Node* function();
static Node* block();
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

// 式の集まり.
Node* code[100];

// トークナイズした結果のトークン列
static Vector* tokens = NULL;

// 現在読んでいるトークンの位置.
static int pos;

// 変数の個数
static size_t count_local_variables;

// 変数名 -> offset
static Map* variable_name_map = NULL;

// user_inputが指している文字列を
// トークンに分割してtokensに保存する
void tokenize()
{
    char* p = user_input;
    tokens = new_vector();

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

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>' || *p == ';' || *p == '=' || *p == '{' || *p == '}' || *p == ',') {
            token->ty = *p;
            token->input = p;
            vec_push(tokens, token);
            p++;
            continue;
        }

        if (isdigit(*p)) {
            token->ty = TK_NUM;
            token->input = p;
            token->val = strtol(p, &p, 10);
            vec_push(tokens, token);
            continue;
        }

        // find the variable name.
        char* name = p;
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
}

// program    = function*
// function   = ident "(" ("int" ident ("," "int" ident)*)* ")" block
// block      = "{" stmt* "}"
// stmt       = "if" "(" expr ")" stmt ("else" stmt)? |
//              "while" "(" expr ")" stmt |
//              "for" "(" expr? ";" expr? ")" stmt |
//              block |
//              "return" expr ";"
//              expr ";"
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? term
// term       = num |
//              ident ("(" (expr ("," expr)*)* ")")? |
//              "int" ident |
//              "(" expr ")
// ident      = chars (chars | num)+
// chars      = [a-zA-Z_]
// num        = [0-9]+
void program()
{
    int i = 0;
    Token** ts = (Token**)(tokens->data);
    while (ts[pos]->ty != TK_EOF) {
        code[i++] = function();
    }
    code[i] = NULL;
}

static Node* function()
{
    Token** ts = (Token**)(tokens->data);
    if (ts[pos]->ty != TK_IDENT) {
        error_at(ts[pos]->input, "先頭が関数名ではない");
    }

    NodeFunction* node_function = malloc(sizeof(NodeFunction));
    node_function->variable_name_map = new_map();
    node_function->count_local_variables = 0;

    // 関数名を取得.
    node_function->name = ts[pos++]->name;

    if (!consume('(')) {
        error_at(ts[pos]->input, "関数の(が無い");
    }

    // 仮引数を読み込む
    while (1) {
        if (consume(')')) {
            break;
        } else if (consume(TK_EOF)) {
            error_at(ts[pos]->input, "関数の)が無い");
        } else {
            if ((ts[pos]->ty != TK_IDENT) || (strcmp(ts[pos]->name, "int") != 0)) {
                error_at(ts[pos]->input, "関数の引数の型がintではない");
            }
            ++pos;

            if (ts[pos]->ty != TK_IDENT) {
                error_at(ts[pos]->input, "関数の引数が識別子ではない");
            }

            // 引数を格納.
            void* offset = (void*)(++node_function->count_local_variables * 8);
            map_put(node_function->variable_name_map, ts[pos++]->name, offset);

            // 引数が更に合ったときのために','を消費しておく.
            consume(',');
        }
    }

    // ローカル変数のparseのために切り替える.
    count_local_variables = node_function->count_local_variables;
    variable_name_map = node_function->variable_name_map;

    Node* node = new_node(ND_FUNCTION, block(), NULL);
    node->type_depend_value = node_function;

    // 戻す.
    node_function->count_local_variables = count_local_variables;
    variable_name_map = NULL;

    return node;
}

static Node* block()
{
    Token** ts = (Token**)(tokens->data);

    if (!consume('{')) {
        error_at(ts[pos]->input, "ブロックの{が必要");
    }

    Vector* stmts = new_vector();
    while (!consume('}')) {
        if (consume(TK_EOF)) {
            error_at(ts[pos]->input, "ブロックが閉じられていない");
        }

        vec_push(stmts, stmt());
    }

    Node* node = new_node(ND_BLOCK, NULL, NULL);
    node->type_depend_value = stmts;

    return node;
}

static Node* stmt()
{
    Node* node;
    Token** ts = (Token**)(tokens->data);

    if (consume(TK_IF)) {
        if (!consume('(')) {
            error_at(ts[pos]->input, "ifの条件部は'('から始まらなくてはならない");
        }

        NodeIfElse* node_if_else = malloc(sizeof(NodeIfElse));
        node_if_else->condition = expr();

        if (!consume(')')) {
            error_at(ts[pos]->input, "ifの条件部は')'で終わらなければならない");
        }

        node_if_else->body = stmt();

        if (consume(TK_ELSE)) {
            node_if_else->else_body = stmt();
        } else {
            node_if_else->else_body = NULL;
        }

        node = malloc(sizeof(Node));
        node->ty = ND_IF;
        node->type_depend_value = node_if_else;
    } else if (consume(TK_WHILE)) {
        if (!consume('(')) {
            error_at(ts[pos]->input, "whileの条件部は'('から始まらなくてはならない");
        }

        node = malloc(sizeof(Node));
        node->ty = ND_WHILE;
        node->lhs = expr(); // 条件式.

        if (!consume(')')) {
            error_at(ts[pos]->input, "whileの条件部は')'で終わらなければならない");
        }

        node->rhs = stmt(); // body
    } else if (consume(TK_FOR)) {
        if (!consume('(')) {
            error_at(ts[pos]->input, "forの直後は'('から始まらなくてはならない");
        }

        NodeFor* node_for = malloc(sizeof(NodeFor));
        if (!consume(';')) {
            node_for->initializing = expr();
            if (!consume(';')) {
                error_at(ts[pos]->input, "';'ではないトークンです");
            }
        } else {
            node_for->initializing = NULL;
        }

        if (!consume(';')) {
            node_for->condition = expr();
            if (!consume(';')) {
                error_at(ts[pos]->input, "';'ではないトークンです");
            }
        } else {
            node_for->condition = NULL;
        }

        if (consume(')')) {
            node_for->updating = NULL;
        } else {
            node_for->updating = expr();
            if (!consume(')')) {
                error_at(ts[pos]->input, "forは')'で終わらなければならない");
            }
        }

        node_for->body = stmt();

        node = malloc(sizeof(Node));
        node->ty = ND_FOR;
        node->type_depend_value = node_for;
    } else if (ts[pos]->ty == '{') {
        node = block();
    } else {
        if (consume(TK_RETURN)) {
            node = malloc(sizeof(Node));
            node->ty = ND_RETURN;
            node->lhs = expr();
        } else {
            node = expr();
        }

        if (!consume(';')) {
            error_at(ts[pos]->input, "';'ではないトークンです");
        }
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

    if (ts[pos]->ty == TK_NUM) {
        // 数値
        return new_node_num(ts[pos++]->val);
    } else if (ts[pos]->ty == TK_IDENT) {
        Node* node = malloc(sizeof(Node));
        char* ident = ts[pos++]->name;

        if (consume('(')) {
            // 関数呼び出し
            NodeCall* node_call = malloc(sizeof(NodeCall));
            node_call->name = ident;
            node_call->arguments = new_vector();

            while (1) {
                if (consume(')')) {
                    break;
                } else {
                    // 引数を格納.
                    vec_push(node_call->arguments, expr());

                    // 引数が更に合ったときのために','を消費しておく.
                    consume(',');
                }
            }

            node->ty = ND_CALL;
            node->type_depend_value = node_call;
        } else if (strcmp(ident, "int") == 0) {
            if (ts[pos]->ty != TK_IDENT) {
                error_at(ts[pos]->input, "変数名が識別子ではない");
            }

            // ローカル変数の宣言.
            ++count_local_variables;
            node->ty = ND_LVAR_NEW;
            node->offset = count_local_variables * 8;
            map_put(variable_name_map, ts[pos++]->name, (void*)node->offset);
        } else {
            // ローカル変数の参照.
            void* offset = map_get(variable_name_map, ident);
            if (NULL == offset) {
                error_at(ts[pos - 1]->input, "宣言されていない変数を使用した");
            }
            node->ty = ND_LVAR;
            node->offset = (uintptr_t)offset;
        }

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
