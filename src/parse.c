#include "9mm.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static Node* global(void);
static Node* function(Type*);
static Node* block(void);
static Node* stmt(void);
static Node* expr(void);
static Node* assign(void);
static Node* equality(void);
static Node* relational(void);
static Node* add(void);
static Node* mul(void);
static Node* unary(void);
static Node* term(void);
static Node* decl_var(Type*);
static Type* parse_type(void);
static bool consume(int);
static bool is_type(void);
static Node* new_node(int, Node*, Node*);
static Node* new_node_num(int);
static Type* new_type(int, Type*);
static inline size_t get_pointed_type_size(Type const*);
static inline Context* new_context(void);
static inline Node* convert_ptr_plus_minus(Node*);

// トークナイズした結果のトークン列
static Vector const* token_vector = NULL;

// 現在読んでいるトークンの位置.
static int pos;

// Current context.
static Context* context = NULL;

// Global variable type map.
static Map* gvar_type_map = NULL;


Node const* const* program(Vector const* tv)
{
    token_vector = tv;

    Token** tokens = (Token**)token_vector->data;

    gvar_type_map = new_map();

    size_t i = 0;
    Node const** code = malloc(sizeof(Node*) * 64);
    while (tokens[pos]->ty != TK_EOF) {
        code[i++] = global();
    }
    code[i] = NULL;

    return code;
}

static Node* global()
{
    Type* type = parse_type();

    Token** tokens = (Token**)(token_vector->data);
    if (tokens[pos]->ty == TK_IDENT && tokens[pos + 1]->ty == '(') {
        // Define function.
        return function(type);
    } else {
        // Declare global variable.
        Node* node = decl_var(type);

        if (!consume(';')) {
            error_at(tokens[pos]->input, "';' is missing");
        }
        return node;
    }
}

static Node* function(Type* type)
{
    Token** tokens = (Token**)(token_vector->data);

    char const* name = tokens[pos++]->name;
    consume('(');

    Node* node = new_node(ND_FUNCTION, NULL, NULL);
    node->rtype = type;
    node->function->name = name;
    node->function->context = new_context();

    // Switch current context.
    Context* prev_context = context;
    context = node->function->context;

    // Parse the arguments of the function.
    while (1) {
        if (consume(')')) {
            break;
        } else if (consume(TK_EOF)) {
            error_at(tokens[pos]->input, "関数の)が無い");
        } else {
            vec_push(node->function->args, decl_var(parse_type()));

            // 引数が更にあったときのために','を消費しておく.
            consume(',');
        }
    }

    // Parse the function body.
    node->lhs = block();

    // Finish the current context;
    context = prev_context;

    return node;
}

static Node* block(void)
{
    Token** tokens = (Token**)(token_vector->data);

    if (!consume('{')) {
        error_at(tokens[pos]->input, "ブロックの{が必要");
    }

    Node* node = new_node(ND_BLOCK, NULL, NULL);
    while (!consume('}')) {
        if (consume(TK_EOF)) {
            error_at(tokens[pos]->input, "ブロックが閉じられていない");
        }

        vec_push(node->stmts, stmt());
    }

    return node;
}

static Node* stmt(void)
{
    Node* node = NULL;
    Token** tokens = (Token**)(token_vector->data);

    if (consume(TK_IF)) {
        if (!consume('(')) {
            error_at(tokens[pos]->input, "ifの条件部は'('から始まらなくてはならない");
        }

        node = new_node(ND_IF, NULL, NULL);
        node->if_else->condition = expr();

        if (!consume(')')) {
            error_at(tokens[pos]->input, "ifの条件部は')'で終わらなければならない");
        }

        node->if_else->body = stmt();

        if (consume(TK_ELSE)) {
            node->if_else->else_body = stmt();
        } else {
            node->if_else->else_body = NULL;
        }
    } else if (consume(TK_WHILE)) {
        if (!consume('(')) {
            error_at(tokens[pos]->input, "whileの条件部は'('から始まらなくてはならない");
        }

        // Condition.
        Node* lhs = expr();

        if (!consume(')')) {
            error_at(tokens[pos]->input, "whileの条件部は')'で終わらなければならない");
        }

        // Body.
        Node* rhs = stmt();

        node = new_node(ND_WHILE, lhs, rhs);
    } else if (consume(TK_FOR)) {
        if (!consume('(')) {
            error_at(tokens[pos]->input, "forの直後は'('から始まらなくてはならない");
        }

        node = new_node(ND_FOR, NULL, NULL);
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
            node = new_node(ND_RETURN, expr(), NULL);
        } else {
            node = expr();
        }

        if (!consume(';')) {
            error_at(tokens[pos]->input, "';'ではないトークンです");
        }
    }

    return node;
}

static Node* expr(void)
{
    return assign();
}

static Node* assign(void)
{
    Node* node = equality();
    if (consume('='))
        node = new_node('=', node, assign());
    return node;
}

static Node* equality(void)
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

static Node* relational(void)
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

static Node* add(void)
{
    Node* node = mul();

    Token** tokens = (Token**)(token_vector->data);
    for (;;) {
        int op = tokens[pos]->ty;

        if (op == '+' || op == '-') {
            ++pos;
            node = new_node(op, node, mul());
            node = convert_ptr_plus_minus(node);
        } else {
            return node;
        }
    }
}

static Node* mul(void)
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

static Node* unary(void)
{
    Token** tokens = (Token**)(token_vector->data);

    if (tokens[pos]->ty == TK_SIZEOF) {
        size_t pos_debug = ++pos;
        Node* node = unary();
        if (node->rtype == NULL) {
            error_at(tokens[pos_debug]->input, "the argument of sizeof is only expression");
        }
        return new_node_num(node->rtype->size);
    } else if (consume('+')) {
        return term();
    } else if (consume('-')) {
        return new_node('-', new_node_num(0), term());
    } else if (consume('*')) {
        return new_node(ND_DEREF, term(), NULL);
    } else if (consume('&')) {
        Node* node = term();
        if (node->ty == ND_DEREF && node->rtype->ty == ARRAY) {
            return node->lhs;
        } else {
            return new_node(ND_REF, node, NULL);
        }
    }
    return term();
}

static Node* term(void)
{
    Token** tokens = (Token**)(token_vector->data);

    if (consume('(')) {
        // For "(" expr ")"
        Node* node = expr();
        if (!consume(')')) {
            error_at(tokens[pos]->input, "')' is missing");
        }
        return node;
    } else if (tokens[pos]->ty == TK_NUM) {
        // For number.
        return new_node_num(tokens[pos++]->val);
    } else if (tokens[pos]->ty == TK_IDENT) {
        char const* ident = tokens[pos++]->name;

        if (consume('(')) {
            // Function call.
            Node* node = new_node(ND_CALL, NULL, NULL);
            node->call->name = ident;
            node->call->arguments = new_vector();

            while (!consume(')')) {
                // Store the argument.
                vec_push(node->call->arguments, expr());

                // For the next argument.
                consume(',');
            }

            return node;
        } else if (--pos, is_type()) {
            return decl_var(parse_type());
        } else {
            // Reference local variable.
            char const* name = tokens[pos++]->name;
            Node* node = NULL;

            Type const* type = map_get(context->var_type_map, name);
            if (type != NULL) {
                node = new_node(ND_LVAR, NULL, NULL);
            } else {
                type = map_get(gvar_type_map, name);
                if (type != NULL) {
                    node = new_node(ND_GVAR, NULL, NULL);
                } else {
                    error_at(tokens[pos - 1]->input, "Not declared variable is used");
                }
            }

            node->name = name;
            node->rtype = type;

            if (consume('[')) {
                // Accessing the array argument via the given index.
                if (node->rtype->ty != ARRAY) {
                    error_at(tokens[pos - 2]->input, "Not array");
                }

                // Accessing the array via the given index.
                Node* node_index_expr = expr();

                if (!consume(']')) {
                    error_at(tokens[pos - 1]->input, "']' is missing");
                }

                node = new_node('+', node, node_index_expr);
                node = convert_ptr_plus_minus(node);

                return new_node(ND_DEREF, node, NULL);
            }

            return node;
        }
    }

    error_at(tokens[pos]->input, "unexpected token is given");

    return NULL;
}

static Node* decl_var(Type* type)
{
    Token** tokens = (Token**)(token_vector->data);
    char const* name = tokens[pos++]->name;

    if (tokens[pos]->ty == '[') {
        // Array type.
        pos++;
        if (tokens[pos]->ty != TK_NUM) {
            error_at(tokens[pos]->input, "it has to be constant number");
        }

        type = new_type(ARRAY, type);
        type->size = tokens[pos]->val * type->ptr_to->size;

        pos++;
        if (tokens[pos++]->ty != ']') {
            error_at(tokens[pos]->input, "missing ] of array");
        }
    }

    if (context != NULL) {
        Node* node = new_node(ND_LVAR_NEW, NULL, NULL);
        node->name = name;
        node->rtype = type;

        context->current_offset += node->rtype->size;
        ++context->count_vars;

        // Store variable info into the current context.
        map_put(context->var_offset_map, name, (void*)context->current_offset);
        map_put(context->var_type_map, name, type);

        return node;
    } else {
        Node* node = new_node(ND_GVAR_NEW, NULL, NULL);
        node->name = name;
        node->rtype = type;

        // Store global variable info.
        map_put(gvar_type_map, name, type);

        return node;
    }
}

static Type* parse_type(void)
{
    Token** tokens = (Token**)(token_vector->data);

    if (tokens[pos]->ty != TK_IDENT) {
        // TODO: Confirm is the given type exist?
        error_at(tokens[pos]->input, "not type");
    }
    pos++;

    Type* type = new_type(INT, NULL);
    while (consume('*')) {
        // ポインタ型の解析
        type = new_type(PTR, type);
    }

    return type;
}

// Return true if the type of a token at the current potions is same to the given type.
// otherwise Return false.
static bool consume(int ty)
{
    Token** tokens = (Token**)(token_vector->data);
    if (tokens[pos]->ty != ty)
        return false;
    pos++;
    return true;
}

static bool is_type(void)
{
    Token** tokens = (Token**)(token_vector->data);
    return tokens[pos]->ty == TK_IDENT && strcmp(tokens[pos]->name, "int") == 0;
}

static Node* new_node(int ty, Node* lhs, Node* rhs)
{
    Node* node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;

    // Allocate the type specific object.
    switch (ty) {
        case ND_FUNCTION:
            node->function = malloc(sizeof(NodeFunction));
            node->function->args = new_vector();
            break;
        case ND_BLOCK:
            node->stmts = new_vector();
            break;
        case ND_IF:
            node->if_else = malloc(sizeof(NodeIfElse));
            break;
        case ND_FOR:
            node->fors = malloc(sizeof(NodeFor));
            break;
        case ND_CALL:
            node->call = malloc(sizeof(NodeCall));
            break;
        default:
            node->tv = NULL;
    }

    // Find the type of result of this node.
    switch (ty) {
        case '=':
            node->rtype = rhs->rtype;
            break;
        case '<':
        case '>':
        case ND_CALL:
        case ND_NUM:
            node->rtype = new_type(INT, NULL);
            break;
        case ND_REF:
            node->rtype = new_type(PTR, NULL);
            break;
        case ND_DEREF:
            node->rtype = lhs->rtype;
            break;
        case '+':
        case '-':
        case '*':
        case '/':
            node->rtype = (lhs->rtype->size < rhs->rtype->size) ? rhs->rtype : lhs->rtype;
            break;
        case ND_LVAR:
        case ND_LVAR_NEW:
        default:
            /* rtype is set outside or unused */
            node->rtype = NULL;
    }

    return node;
}

static Node* new_node_num(int val)
{
    Node* node = new_node(ND_NUM, NULL, NULL);
    node->val = val;
    return node;
}

static Type* new_type(int ty, Type* ptr_to)
{
    Type* type = malloc(sizeof(Type));
    type->ty = ty;
    type->ptr_to = ptr_to;

    switch (ty) {
        case INT:
            type->size = 4;
            break;
        case PTR:
            type->size = 8;
            break;
        default:
            // Set the size by yourself.
            type->size = 0;
            break;
    }

    return type;
}

static inline size_t get_pointed_type_size(Type const* type)
{
    if (type == NULL) {
        error("NULL is given");
    }

    if (type->ty != PTR && type->ty != ARRAY) {
        error("Not pointer type is given");
    }

    if (type->ptr_to == NULL) {
        error("Pointer points to NULL");
    }

    switch (type->ptr_to->ty) {
        case INT:
            return 4;
        case PTR:
        case ARRAY:
            return 8;
        default:
            error("unknown type");
    }
}

static inline Context* new_context(void)
{
    Context* context = malloc(sizeof(Context));

    context->count_vars = 0;
    context->current_offset = 0;
    context->var_offset_map = new_map();
    context->var_type_map = new_map();

    return context;
}

static Node* convert_ptr_plus_minus(Node* node)
{
    if (node->ty != '+' && node->ty != '-') {
        error("Converting pointer calculation is for only '+' and '-'");
    }

    Token** tokens = (Token**)(token_vector->data);
    Node* lhs = node->lhs;
    Node* rhs = node->rhs;

    // Add/Sub for pointer or array type.
    if (lhs->ty == ND_LVAR && rhs->ty == ND_NUM) {
        // p + 1 -> p + (1 * sizeof(p))
        // p - 1 -> p - (1 * sizeof(p))
        Type const* type = map_get(context->var_type_map, lhs->name);
        if (type == NULL) {
            error_at(tokens[pos]->input, "undefined variable");
        }

        if (type->ty == PTR || type->ty == ARRAY) {
            node->rhs = new_node('*', rhs, new_node_num(get_pointed_type_size(type)));
        }
    } else if (lhs->ty == ND_NUM && rhs->ty == ND_LVAR) {
        // 1 + p -> (1 * sizeof(p)) + p
        // 1 - p -> (1 * sizeof(p)) - p (FORBIDDEN)
        Type const* type = map_get(context->var_type_map, rhs->name);
        if (type == NULL) {
            error_at(tokens[pos]->input, "undefined variable");
        }

        if (node->ty == '-') {
            error_at(tokens[pos]->input, "invalid operand");
        }

        if (type->ty == PTR || type->ty == ARRAY) {
            node->lhs = new_node('*', lhs, new_node_num(get_pointed_type_size(type)));
        }
    }

    return node;
}
