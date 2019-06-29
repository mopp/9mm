#include "9mm.h"
#include <stdlib.h>
#include <string.h>

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
static Node* decl_var();
static int consume(int);
static Node* new_node(int, Node*, Node*);
static Node* new_node_num(int);
static int is_type();
static inline Context* new_context();

// 式の集まり.
Node* code[100];

// トークナイズした結果のトークン列
static Vector const* token_vector = NULL;

// 現在読んでいるトークンの位置.
static int pos;

// Current context.
static Context* context = NULL;


Node const* const* program(Vector const* tv)
{
    token_vector = tv;

    Token** tokens = (Token**)token_vector->data;

    size_t i = 0;
    Node const** code = malloc(sizeof(Node*) * 64);
    while (tokens[pos]->ty != TK_EOF) {
        code[i++] = function();
    }
    code[i] = NULL;
    return code;
}

static Node* function()
{
    Token** tokens = (Token**)(token_vector->data);

    if (tokens[pos]->ty != TK_IDENT || strcmp(tokens[pos]->name, "int") != 0) {
        error_at(tokens[pos]->input, "関数の返り値がintではない");
    }
    ++pos;

    if (tokens[pos]->ty != TK_IDENT) {
        error_at(tokens[pos]->input, "先頭が関数名ではない");
    }

    char const* name = tokens[pos++]->name;

    if (!consume('(')) {
        error_at(tokens[pos]->input, "関数の(が無い");
    }

    Node* node = new_node(ND_FUNCTION, NULL, NULL);
    node->function = malloc(sizeof(NodeFunction));
    node->function->name = name;
    node->function->context = new_context();

    // Switch current context.
    context = node->function->context;

    // Parse the arguments of the function.
    while (1) {
        if (consume(')')) {
            break;
        } else if (consume(TK_EOF)) {
            error_at(tokens[pos]->input, "関数の)が無い");
        } else {
            free(decl_var());

            // 引数が更にあったときのために','を消費しておく.
            consume(',');
        }
    }

    // Parse the function body.
    node->lhs = block();

    // Finish the current context;
    context = NULL;

    return node;
}

static Node* block()
{
    Token** tokens = (Token**)(token_vector->data);

    if (!consume('{')) {
        error_at(tokens[pos]->input, "ブロックの{が必要");
    }

    Node* node = new_node(ND_BLOCK, NULL, NULL);
    node->stmts = new_vector();
    while (!consume('}')) {
        if (consume(TK_EOF)) {
            error_at(tokens[pos]->input, "ブロックが閉じられていない");
        }

        vec_push(node->stmts, stmt());
    }

    return node;
}

static Node* stmt()
{
    Node* node;
    Token** tokens = (Token**)(token_vector->data);

    if (consume(TK_IF)) {
        if (!consume('(')) {
            error_at(tokens[pos]->input, "ifの条件部は'('から始まらなくてはならない");
        }

        NodeIfElse* node_if_else = malloc(sizeof(NodeIfElse));
        node_if_else->condition = expr();

        if (!consume(')')) {
            error_at(tokens[pos]->input, "ifの条件部は')'で終わらなければならない");
        }

        node_if_else->body = stmt();

        if (consume(TK_ELSE)) {
            node_if_else->else_body = stmt();
        } else {
            node_if_else->else_body = NULL;
        }

        node = malloc(sizeof(Node));
        node->ty = ND_IF;
        node->if_else = node_if_else;
    } else if (consume(TK_WHILE)) {
        if (!consume('(')) {
            error_at(tokens[pos]->input, "whileの条件部は'('から始まらなくてはならない");
        }

        node = malloc(sizeof(Node));
        node->ty = ND_WHILE;
        node->lhs = expr(); // 条件式.

        if (!consume(')')) {
            error_at(tokens[pos]->input, "whileの条件部は')'で終わらなければならない");
        }

        node->rhs = stmt(); // body
    } else if (consume(TK_FOR)) {
        if (!consume('(')) {
            error_at(tokens[pos]->input, "forの直後は'('から始まらなくてはならない");
        }

        node = malloc(sizeof(Node));
        node->ty = ND_FOR;
        node->fors = malloc(sizeof(NodeFor));
        if (!consume(';')) {
            node->fors->initializing = expr();
            if (!consume(';')) {
                error_at(tokens[pos]->input, "';'ではないトークンです");
            }
        } else {
            node->fors->initializing = NULL;
        }

        if (!consume(';')) {
            node->fors->condition = expr();
            if (!consume(';')) {
                error_at(tokens[pos]->input, "';'ではないトークンです");
            }
        } else {
            node->fors->condition = NULL;
        }

        if (consume(')')) {
            node->fors->updating = NULL;
        } else {
            node->fors->updating = expr();
            if (!consume(')')) {
                error_at(tokens[pos]->input, "forは')'で終わらなければならない");
            }
        }

        node->fors->body = stmt();
    } else if (tokens[pos]->ty == '{') {
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
            error_at(tokens[pos]->input, "';'ではないトークンです");
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
    if (consume('*'))
        return new_node(ND_DEREF, term(), NULL);
    if (consume('&'))
        return new_node(ND_REF, term(), NULL);
    return term();
}

static Node* term()
{
    Token** tokens = (Token**)(token_vector->data);

    if (consume('(')) {
        // 次のトークンが'('なら、"(" expr ")"のはず
        Node* node = expr();
        if (!consume(')'))
            error_at(tokens[pos]->input,
                     "開きカッコに対応する閉じカッコがありません");
        return node;
    } else if (tokens[pos]->ty == TK_NUM) {
        // 数値
        return new_node_num(tokens[pos++]->val);
    } else if (tokens[pos]->ty == TK_IDENT) {
        Node* node = malloc(sizeof(Node));
        char const* ident = tokens[pos++]->name;

        if (consume('(')) {
            // 関数呼び出し
            node->ty = ND_CALL;
            node->call = malloc(sizeof(NodeCall));
            node->call->name = ident;
            node->call->arguments = new_vector();

            while (1) {
                if (consume(')')) {
                    break;
                } else {
                    // 引数を格納.
                    vec_push(node->call->arguments, expr());

                    // 引数が更に合ったときのために','を消費しておく.
                    consume(',');
                }
            }
        } else if (--pos, is_type()) {
            free(node);
            node = decl_var();
        } else {
            // ローカル変数の参照.
            void const* offset = map_get(context->var_offset_map, tokens[pos++]->name);
            if (NULL == offset) {
                error_at(tokens[pos - 1]->input, "宣言されていない変数を使用した");
            }
            node->ty = ND_LVAR;
            node->offset = (uintptr_t)offset;
        }

        return node;
    }

    error_at(tokens[pos]->input,
             "数値でも開きカッコでもないトークンです");

    return NULL;
}

static Node* decl_var()
{
    Token** tokens = (Token**)(token_vector->data);

    if (tokens[pos]->ty != TK_IDENT) {
        error_at(tokens[pos]->input, "not type");
    }
    pos++;

    Type* type = malloc(sizeof(Type));
    type->ty = INT;
    type->ptr_to = NULL;
    while (consume('*')) {
        // ポインタ型の解析
        Type* t = malloc(sizeof(Type));
        t->ty = PTR;
        t->ptr_to = type;
        type = t;
    }

    if (tokens[pos]->ty != TK_IDENT) {
        error_at(tokens[pos]->input, "invalid variable name");
    }

    Node* node = new_node(ND_LVAR_NEW, NULL, NULL);
    node->offset = ++context->count_vars * 8;

    // Store variable info into the current context.
    char const* name = tokens[pos++]->name;
    map_put(context->var_offset_map, name, (void*)node->offset);
    map_put(context->var_type_map, name, type);

    return node;
}

// 期待した型であれば1トークン読み進める
// 成功したら1が返る
static int consume(int ty)
{
    Token** tokens = (Token**)(token_vector->data);
    if (tokens[pos]->ty != ty)
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

static int is_type()
{
    Token** tokens = (Token**)(token_vector->data);
    return tokens[pos]->ty == TK_IDENT && strcmp(tokens[pos]->name, "int") == 0;
}

static inline Context* new_context()
{
    Context* context = malloc(sizeof(Context));

    context->count_vars = 0;
    context->var_offset_map = new_map();
    context->var_type_map = new_map();

    return context;
}
