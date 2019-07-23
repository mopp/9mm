#include "9mm.h"

#ifndef SELFHOST_9MM
static Node* global(void);
static Node* function(Type*);
static void strut(void);
static void enm(void);
static Node* block(void);
static Node* stmt(void);
static Node* expr(void);
static Node* assign(void);
static Node* and_or(void);
static Node* equality(void);
static Node* relational(void);
static Node* add(void);
static Node* mul(void);
static Node* unary(void);
static Node* term(void);
static Node* decl_var(Type*);
static Node* ref_var(void);
static Type* parse_type(void);
static int consume(int);
static Node* new_node(int, Node*, Node*);
static Node* new_node_num(size_t);
static Type* new_type(int, Type const*);
static Type* new_user_type(UserType*);
static size_t get_type_size(Type const*);
static Context* new_context(void);
static Node* convert_ptr_plus_minus(Node*);
#endif

// トークナイズした結果のトークン列
static Vector const* token_vector;

// 現在読んでいるトークンの位置.
static int pos;

// Current context.
static Context* context;

// Global variable type map.
static Map* gvar_type_map;

// String literal to label map.
static Map* str_label_map;

// Name to user defined type map.
static Map* user_types;

// Enum member to number.
static Map* enum_map;

Code const* program(Vector const* tv)
{
    token_vector = tv;

    Token** tokens = (Token**)token_vector->data;

    gvar_type_map = new_map();
    str_label_map = new_map();
    user_types = new_map();
    enum_map = new_map();

    size_t count = 0;
    Node const** asts = malloc(sizeof(Node*) * 64);
    while (tokens[pos]->ty != TK_EOF) {
        asts[count++] = global();
    }

    Code* code = malloc(sizeof(Code));
    code->asts = asts;
    code->count_ast = count;
    code->str_label_map = str_label_map;

    return code;
}

static Node* global()
{
    Token** tokens = (Token**)(token_vector->data);
    if (tokens[pos]->ty == TK_STRUCT && tokens[pos + 1]->ty == TK_IDENT && (tokens[pos + 2]->ty == '{' || tokens[pos + 2]->ty == ';')) {
        strut();
        return global();
    } else if (tokens[pos]->ty == TK_ENUM) {
        enm();
        return global();
    } else if (consume(TK_TYPEDEF)) {
        if (!consume(TK_STRUCT)) {
            error_at(tokens[pos]->input, "typedef for struct is only supported");
        }

        if (!consume(TK_IDENT)) {
            error_at(tokens[pos]->input, "not identifier");
        }

        UserType* user_type = map_get(user_types, tokens[pos - 1]->name);
        if (user_type == NULL) {
            error_at(tokens[pos]->input, "undeclared type");
        }

        if (!consume(TK_IDENT)) {
            error_at(tokens[pos]->input, "not identifier");
        }

        map_put(user_types, tokens[pos - 1]->name, user_type);

        if (!consume(';')) {
            error_at(tokens[pos]->input, "';' is missing");
        }

        return global();
    } else if (consume(TK_EXTERN)) {
        Type* type = parse_type();
        if (type == NULL) {
            error_at(tokens[pos]->input, "undeclared type");
        }
        if (!consume(TK_IDENT)) {
            error_at(tokens[pos]->input, "variable has to be identifier");
        }

        map_put(gvar_type_map, tokens[pos - 1]->name, type);

        if (!consume(';')) {
            error_at(tokens[pos]->input, "';' is missing");
        }
        return global();
    }

    Type* type = parse_type();

    if (tokens[pos]->ty == TK_IDENT && tokens[pos + 1]->ty == '(') {
        // Define function.
        return function(type);
    } else {
        // FIXME: Investigate why I need this cleanup...
        context = NULL;

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
    while (!consume(')')) {
        if (consume(TK_EOF)) {
            error_at(tokens[pos]->input, "関数の)が無い");
        } else if (strcmp(tokens[pos]->name, "void") == 0 && tokens[pos + 1]->ty == ')') {
            pos += 2;
            break;
        } else {
            vec_push(node->function->args, decl_var(parse_type()));

            // 引数が更にあったときのために','を消費しておく.
            consume(',');
        }
    }

    if (consume(';')) {
        // Function prototype.
        // FIXME: store function table.
        node->lhs = NULL;
    } else {
        // Parse the function body.
        node->lhs = block();
    }


    // Finish the current context;
    context = prev_context;

    return node;
}

static void enm(void)
{
    if (!consume(TK_ENUM)) {
        error("not enum");
    }

    Token** tokens = (Token**)(token_vector->data);
    if (!consume('{')) {
        error_at(tokens[pos]->input, "'{' is missing");
    }

    // Start from 1 to distinguish NULL in struct map.
    size_t count = 1;
    while (1) {
        if (tokens[pos]->ty == TK_IDENT) {
            char const* name = tokens[pos++]->name;
            if (consume('=')) {
                count = tokens[pos++]->val + 1;
            }
            map_put(enum_map, name, (void*)count);
            ++count;

            consume(',');
        } else {
            error_at(tokens[pos]->input, "enum member has to be identifier");
        }

        if (consume('}') || consume(TK_EOF)) {
            break;
        }
    }

    if (!consume(';')) {
        error_at(tokens[pos]->input, "';' is missing");
    }
}

static void strut(void)
{
    if (!consume(TK_STRUCT)) {
        error("not struct");
    }

    Token** tokens = (Token**)(token_vector->data);
    if (tokens[pos]->ty != TK_IDENT) {
        error_at(tokens[pos]->input, "struct name has to be identifier");
    }

    UserType* user_type = malloc(sizeof(UserType));
    user_type->name = tokens[pos++]->name;
    user_type->size = 0;
    user_type->member_offset_map = new_map();
    user_type->member_type_map = new_map();

    // Put it here for self pointer struct.
    map_put(user_types, user_type->name, user_type);

    if (consume(';')) {
        // Opaque struct declaration.
        return;
    }

    if (!consume('{')) {
        error_at(tokens[pos]->input, "You need { here");
    }

    // FIXME: Consider padding.
    while (!consume('}')) {
        Type* member_type = parse_type();
        if (member_type == NULL) {
            error_at(tokens[pos]->input, "undeclared type");
        }

        if (tokens[pos]->ty != TK_IDENT) {
            error_at(tokens[pos]->input, "struct name has to be identifier");
        }

        char const* member_name = tokens[pos++]->name;
        map_put(user_type->member_offset_map, member_name, (void*)user_type->size);
        map_put(user_type->member_type_map, member_name, member_type);

        user_type->size += member_type->size;

        if (!consume(';')) {
            error_at(tokens[pos]->input, "';' is required");
        }
    }

    if (!consume(';')) {
        error_at(tokens[pos]->input, "';' is required");
    }

    if (user_type->member_offset_map->keys->len == 0) {
        error_at(tokens[pos]->input, "struct must have a field at least");
    }
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

        char* break_label = malloc(sizeof(char) * 128);
        sprintf(break_label, ".L_while_end_%p", lhs);

        char const* prev = context->break_label;
        context->break_label = break_label;

        // Body.
        Node* rhs = stmt();

        context->break_label = prev;

        node = new_node(ND_WHILE, lhs, rhs);
        node->break_label = break_label;
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

        char* break_label = malloc(sizeof(char) * 128);
        sprintf(break_label, ".L_for_end_%p", node);

        char const* prev = context->break_label;
        context->break_label = break_label;

        node->fors->body = stmt();
        node->fors->break_label = break_label;

        context->break_label = prev;
    } else if (tokens[pos]->ty == '{') {
        node = block();
    } else {
        if (consume(TK_RETURN)) {
            if (tokens[pos]->ty == ';') {
                // return empty.
                node = new_node(ND_RETURN, new_node_num(0), NULL);
            } else {
                // return value.
                node = new_node(ND_RETURN, expr(), NULL);
            }
        } else if (consume(TK_BREAK)) {
            node = new_node(ND_BREAK, NULL, NULL);
            node->break_label = context->break_label;
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
    Token** tokens = (Token**)token_vector->data;
    size_t prev_pos = pos;

    Type* type = NULL;
    if (consume('(')) {
        type = parse_type();
        if (type != NULL) {
            if (!consume(')')) {
                error_at(tokens[pos]->input, "')' is missing");
            }
        } else {
            pos = prev_pos;
        }
    }

    Node* node = assign();
    if (type != NULL) {
        node->rtype = type;
    }
    return node;
}

static Node* assign(void)
{
    Node* node = and_or();
    if (consume('=')) {
        if (node->ty == ND_LVAR_NEW) {
            // int x = 3; -> int x; x = 3;
            // Convert ND_LVAR_NEW to ND_LVAR.
            Node* node_lvar = new_node(ND_LVAR, NULL, NULL);
            node_lvar->name = node->name;
            node_lvar->rtype = node->rtype;

            node = new_node(ND_INIT, node, new_node('=', node_lvar, expr()));
        } else {
            node = new_node('=', node, expr());
        }
    } else if (consume(TK_ADD_ASIGN)) {
        // x += 1;
        node = new_node('=', node, new_node('+', node, expr()));
    } else if (consume(TK_SUB_ASIGN)) {
        // x -= 1;
        node = new_node('=', node, new_node('-', node, expr()));
    } else if (consume(TK_MUL_ASIGN)) {
        // x *= 1;
        node = new_node('=', node, new_node('*', node, expr()));
    } else if (consume(TK_DIV_ASIGN)) {
        // x /= 1;
        node = new_node('=', node, new_node('/', node, expr()));
    }

    return node;
}

static Node* and_or(void)
{
    Node* node = equality();
    if (consume(TK_AND)) {
        node = new_node(ND_AND, node, and_or());
    } else if (consume(TK_OR)) {
        node = new_node(ND_OR, node, and_or());
    }
    return node;
}

static Node* equality(void)
{
    Node* node = relational();

    for (;;) {
        if (consume(TK_EQ))
            node = new_node(ND_EQ, node, relational());
        else if (consume(TK_NE))
            node = new_node(ND_NE, node, relational());
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
        size_t prev_pos = ++pos;

        if (!consume('(')) {
            error_at(tokens[pos]->input, "'(' is missing");
        }

        Type* type = parse_type();
        if (type != NULL) {
            if (!consume(')')) {
                error_at(tokens[pos]->input, "')' is missing");
            }

            return new_node_num(type->size);
        }

        pos = prev_pos;
        Node* node = unary();
        if (node->rtype == NULL) {
            error_at(tokens[prev_pos]->input, "the argument of sizeof is only expression or type");
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
    } else if (consume('!')) {
        return new_node('!', term(), NULL);
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
    } else if (tokens[pos]->ty == TK_IDENT && tokens[pos + 1]->ty == '(') {
        // Function call.
        Node* node = new_node(ND_CALL, NULL, NULL);
        node->call->name = tokens[pos]->name;
        node->call->arguments = new_vector();

        pos += 2;

        while (!consume(')')) {
            // Store the argument.
            vec_push(node->call->arguments, expr());

            // For the next argument.
            consume(',');
        }

        return node;
    } else if (tokens[pos]->ty == TK_IDENT || tokens[pos]->ty == TK_STRUCT) {
        // FIXME: Re-design type declaration.
        size_t prev_pos = pos;
        Type* type = parse_type();
        if (type != NULL && tokens[pos]->ty == TK_IDENT) {
            // Next token has to be identifier to declare variable.
            return decl_var(type);
        } else {
            pos = prev_pos;
            return ref_var();
        }
    } else if (tokens[pos]->ty == TK_STR) {
        Node* node = new_node(ND_STR, NULL, NULL);

        char* buf = malloc(64);
        sprintf(buf, "str_%zd", str_label_map->keys->len);
        map_put(str_label_map, tokens[pos]->name, buf);

        node->label = buf;

        pos++;

        return node;
    } else if (tokens[pos]->ty == TK_INCL) {
        // ++i; -> i = i + 1;
        // ++a[i]; -> *(a + i) = *(a + i) + 1;
        pos++;
        Node* n = ref_var();
        return new_node('=', n, new_node('+', n, new_node_num(1)));
    } else if (tokens[pos]->ty == TK_DECL) {
        // --i; -> i = i - 1;
        // --a[i]; -> *(a + i) = *(a + i) - 1;
        pos++;
        Node* n = ref_var();
        return new_node('=', n, new_node('-', n, new_node_num(1)));
    }

    error_at(tokens[pos]->input, "unexpected token is given");

    return NULL;
}

static Node* decl_var(Type* type)
{
    error_if_null(type);

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

        if (node->rtype->size == 0) {
            error_at(tokens[pos - 1]->input, "the size of type is zero, cannot allocate the space");
        }

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

static Node* ref_var(void)
{
    Token** tokens = (Token**)(token_vector->data);

    if (tokens[pos]->ty != TK_IDENT) {
        error_at(tokens[pos]->input, "Not variable name");
    }

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
            size_t n = (size_t)map_get(enum_map, name);
            if (n != 0) {
                return new_node_num(n - 1);
            } else {
                error_at(tokens[pos - 1]->input, "Not declared variable is used");
            }
        }
    }

    node->name = name;
    node->rtype = type;

    while (1) {
        if (consume('.')) {
            // obj.x
            if (tokens[pos]->ty != TK_IDENT) {
                error_at(tokens[pos]->input, "member name has to be identifier");
            }
            UserType* user_type = node->rtype->user_type;
            error_if_null(user_type);

            char const* member_name = tokens[pos++]->name;
            size_t offset = (size_t)map_get(user_type->member_offset_map, member_name);

            Type* member_type = map_get(user_type->member_type_map, member_name);
            error_if_null(member_type);

            node = new_node(ND_DOT_REF, node, NULL);
            node->member_offset = offset;
            node->rtype = member_type;
        } else if (consume(TK_ARROW)) {
            // obj->x -> (*obj).x
            if (tokens[pos]->ty != TK_IDENT) {
                error_at(tokens[pos]->input, "member name has to be identifier");
            }

            if (node->rtype->ty != PTR) {
                error_at(tokens[pos - 2]->input, "the variable is not pointer");
            }

            UserType* user_type = node->rtype->ptr_to->user_type;
            error_if_null(user_type);

            char const* member_name = tokens[pos++]->name;
            size_t offset = (size_t)map_get(user_type->member_offset_map, member_name);

            Type* member_type = map_get(user_type->member_type_map, member_name);
            error_if_null(member_type);

            node = new_node(ND_ARROW_REF, new_node(ND_DEREF, node, NULL), NULL);
            node->member_offset = offset;
            node->rtype = member_type;
        } else {
            type = node->rtype;
            if (consume('[')) {
                // Accessing the array argument via the given index.
                if (node->rtype->ty != ARRAY && node->rtype->ty != PTR) {
                    error_at(tokens[pos - 2]->input, "Array or pointer only can be accessed via index");
                }

                // Accessing the array via the given index.
                Node* node_index_expr = expr();

                if (!consume(']')) {
                    error_at(tokens[pos - 1]->input, "']' is missing");
                }

                // a[0] -> *(a + 0).
                // obj.x[0] -> *(obj.x + 0).
                node = new_node('+', node, node_index_expr);
                node->rtype = type;

                node = new_node(ND_DEREF, node, NULL);
            } else {
                break;
            }
        }
    }

    if (consume(TK_INCL)) {
        // i++ -> tmp = i, i = i + 1, i
        Node* update_node = new_node('=', node, new_node('+', node, new_node_num(1)));
        return new_node(ND_INCL_POST, node, update_node);
    } else if (consume(TK_DECL)) {
        // i-- -> tmp = i, i = i - 1, i
        Node* update_node = new_node('=', node, new_node('-', node, new_node_num(1)));
        return new_node(ND_DECL_POST, node, update_node);
    }

    return node;
}

static Type* parse_type(void)
{
    Token** tokens = (Token**)(token_vector->data);
    size_t prev_pos = pos;

    // Ignore static.
    if (tokens[pos]->ty == TK_IDENT && strcmp(tokens[pos]->name, "static") == 0) {
        // FIXME: handle static identifier.
        ++pos;
    }

    consume(TK_STRUCT);

    Type* type = NULL;
    if (consume(TK_IDENT)) {
        // Declare primitive type.
        char const* name = tokens[pos - 1]->name;
        error_if_null(name);

        if (strcmp(name, "char") == 0) {
            type = new_type(CHAR, NULL);
        } else if (strcmp(name, "int") == 0) {
            type = new_type(INT, NULL);
        } else if (strcmp(name, "void") == 0) {
            type = new_type(VOID, NULL);
        } else if (strcmp(name, "size_t") == 0) {
            type = new_type(SIZE_T, NULL);
        } else {
            UserType* user_type = map_get(user_types, name);
            if (user_type != NULL) {
                type = new_user_type(user_type);
            }
        }
    }

    if (type == NULL) {
        pos = prev_pos;
        return NULL;
    }

    while (1) {
        // Ignore const.
        if (tokens[pos]->ty == TK_IDENT && strcmp(tokens[pos]->name, "const") == 0) {
            // FIXME: handle const type.
            ++pos;
        }

        // Check wheather the type is pointer or not.
        if (!consume('*')) {
            break;
        }

        type = new_type(PTR, type);
    }

    return type;
}

// Return 1 if the type of a token at the current potions is same to the given type.
// otherwise Return 0.
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

    // Allocate the type specific object.
    if (ty == ND_FUNCTION) {
        node->function = malloc(sizeof(NodeFunction));
        node->function->args = new_vector();
    } else if (ty == ND_FUNCTION || ty == ND_BLOCK) {
        node->stmts = new_vector();
    } else if (ty == ND_IF) {
        node->if_else = malloc(sizeof(NodeIfElse));
    } else if (ty == ND_FOR) {
        node->fors = malloc(sizeof(NodeFor));
    } else if (ty == ND_CALL) {
        node->call = malloc(sizeof(NodeCall));
    } else {
        node->tv = NULL;
    }

    // Find the type of result of this node.
    if (ty == '=') {
        node->rtype = lhs->rtype;
    } else if (ty == '<' || ty == '>' ||
               ty == '!' || ty == ND_CALL ||
               ty == ND_NUM || ty == ND_AND ||
               ty == ND_OR || ty == ND_NE ||
               ty == ND_EQ) {
        node->rtype = new_type(INT, NULL);
    } else if (ty == ND_REF) {
        node->rtype = new_type(PTR, node->lhs->rtype);
    } else if (ty == ND_STR) {
        node->rtype = new_type(PTR, new_type(CHAR, NULL));
    } else if (ty == ND_DEREF) {
        if (lhs->rtype->ptr_to == NULL) {
            error("You can dereference only pointer or array");
        }
        node->rtype = lhs->rtype->ptr_to;
    } else if (ty == ND_RETURN) {

        node->rtype = lhs->rtype;
    } else if (ty == '+' || ty == '-' ||
               ty == '*' || ty == '/') {
        error_if_null(lhs->rtype);
        error_if_null(rhs->rtype);
        if (lhs->rtype->size < rhs->rtype->size) {
            node->rtype = rhs->rtype;
        } else {
            node->rtype = lhs->rtype;
        }
    } else if (ty == ND_INCL_POST || ty == ND_DECL_POST) {
        node->rtype = rhs->rtype;
    } else {
        /* rtype is set outside or unused */
        node->rtype = NULL;
        return convert_ptr_plus_minus(node);
    }

    error_if_null(node->rtype);

    return convert_ptr_plus_minus(node);
}

static Node* new_node_num(size_t val)
{
    Node* node = new_node(ND_NUM, NULL, NULL);
    node->val = val;
    return node;
}

static Type* new_type(int ty, Type const* ptr_to)
{
    Type* type = malloc(sizeof(Type));
    type->ty = ty;
    type->ptr_to = ptr_to;
    type->size = get_type_size(type);
    type->user_type = NULL;

    return type;
}

static Type* new_user_type(UserType* user_type)
{
    Type* type = new_type(USER, NULL);
    type->user_type = user_type;
    type->size = user_type->size;

    return type;
}

static size_t get_type_size(Type const* type)
{
    error_if_null(type);

    int ty = type->ty;
    if (ty == CHAR) {
        return 1;
    } else if (ty == INT) {
        return 4;
    } else if (ty == PTR || ty == SIZE_T) {
        return 8;
    } else if (ty == ARRAY || ty == USER || ty == VOID) {
        // Size of array is set by yourself.
        return 0;
    } else {
        error("unknown type");
    }
}

static Context* new_context(void)
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
        return node;
    }

    Node* lhs = node->lhs;
    Node* rhs = node->rhs;

    // Add/Sub for pointer or array type.
    if ((lhs->rtype->ty == PTR || lhs->rtype->ty == ARRAY) && (rhs->rtype->ty == INT || rhs->rtype->ty == SIZE_T)) {
        // p + 1 -> p + (1 * sizeof(p))
        // p - 1 -> p - (1 * sizeof(p))
        error_if_null(lhs->rtype->ptr_to);

        node->rhs = new_node('*', rhs, new_node_num(get_type_size(lhs->rtype->ptr_to)));
    } else if ((lhs->rtype->ty == INT || lhs->rtype->ty == SIZE_T) && (rhs->rtype->ty == PTR || rhs->rtype->ty == ARRAY)) {
        // 1 + p -> (1 * sizeof(p)) + p
        // 1 - p -> (1 * sizeof(p)) - p (FORBIDDEN)
        error_if_null(rhs->rtype->ptr_to);

        if (node->ty == '-') {
            Token** tokens = (Token**)token_vector->data;
            error_at(tokens[pos]->input, "invalid operand");
        }

        node->lhs = new_node('*', lhs, new_node_num(get_type_size(rhs->rtype->ptr_to)));
    }

    return node;
}
