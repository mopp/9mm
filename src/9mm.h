#define _XOPEN_SOURCE 700

#include "container.h"
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    // Type of abstract syntax tree
    ND_NUM = 256, // Constant integer
    ND_RETURN,
    ND_LVAR_NEW,  // Declare local variable
    ND_LVAR,      // Reference local variable
    ND_GVAR_NEW,  // Declare global variable
    ND_GVAR,      // Reference global variable
    ND_IF,        // if
    ND_WHILE,     // while
    ND_FOR,       // for
    ND_BLOCK,     // "{" stmt* "}"
    ND_CALL,      // Call function
    ND_FUNCTION,  // Define function
    ND_REF,       // Reference variable
    ND_DEREF,     // Dereference variable
    ND_STR,       // String literal
    ND_INCL_POST, // i++
    ND_DECL_POST, // i--
    ND_AND,       // &&
    ND_OR,        // ||
    ND_DOT_REF,   // obj.x
    ND_ARROW_REF, // obj->x
    ND_INIT,      // int x = 3
    ND_BREAK,     // break
    ND_EQ,        // ==
    ND_NE,        // !=

    // Type of token
    TK_RETURN,
    TK_IDENT,     // Identifier
    TK_NUM,       // Integer
    TK_EOF,       // End of file
    TK_IF,        // if
    TK_ELSE,      // else
    TK_WHILE,     // while
    TK_FOR,       // for
    TK_EQ,        // ==
    TK_NE,        // !=
    TK_LE,        // <=
    TK_GE,        // >=
    TK_SIZEOF,    // sizeof
    TK_STR,       // String literal
    TK_INCL,      // ++
    TK_DECL,      // --
    TK_AND,       // &&
    TK_OR,        // ||
    TK_STRUCT,    // struct
    TK_ARROW,     // obj->x
    TK_ADD_ASIGN, // +=
    TK_SUB_ASIGN, // -=
    TK_MUL_ASIGN, // *=
    TK_DIV_ASIGN, // /=
    TK_BREAK,     // break
    TK_ENUM,      // enum
    TK_TYPEDEF,   // typedef
    TK_EXTERN     // extern
};

struct token {
    int ty; // Type of token
    // union {
        size_t val;       // value for TK_NUM
        char const* name; // name for TK_IDENT
    // };
    char const* input; // Token string for error message
};
typedef struct token Token;

struct context {
    size_t count_vars;
    size_t current_offset;   // The offset of the current "rbp" register.
    Map* var_offset_map;     // variable name -> offset.
    Map* var_type_map;       // variable name -> "Type".
    char const* break_label; // Label for break in current loop
};
typedef struct context Context;

struct node_function {
    char const* name;
    Vector* args;
    Context* context;
};
typedef struct node_function NodeFunction;

struct node;

struct node_if_else {
    struct node* condition;
    struct node* body;
    struct node* else_body; // NULL means does not have else clause.
};
typedef struct node_if_else NodeIfElse;

struct node_for {
    struct node* initializing;
    struct node* condition;
    struct node* updating;
    struct node* body;
    char const* break_label;
};
typedef struct node_for NodeFor;

struct node_call {
    char const* name;
    Vector* arguments;
};
typedef struct node_call NodeCall;

struct user_type {
    char const* name;
    size_t size;
    Map* member_offset_map; // name -> its offset
    Map* member_type_map;   // name -> its type
};
typedef struct user_type UserType;

enum {
    CHAR,
    INT,
    PTR,
    ARRAY,
    USER,
    VOID,
    SIZE_T
};

struct type {
    int ty;
    struct type const* ptr_to;
    size_t size;
    UserType* user_type; // Valid if ty is USER
};
typedef struct type Type;

struct node {
    int ty;            // Type of "Node"
    struct node* lhs;  // Left-hand-side
    struct node* rhs;  // Right-hand-size
    Type const* rtype; // Type of result of "expr" of "Node".
    // union {
        size_t val;           // for "ND_NUM"
        char const* name;     // for "ND_LVAR"
        size_t member_offset; // for "ND_DOT_REF"
        NodeFunction* function;
        Vector* stmts;
        NodeIfElse* if_else;
        NodeFor* fors;
        NodeCall* call;
        char const* label; // for "ND_STR"
        char const* break_label;
        void* tv;
    // };
};
typedef struct node Node;

struct code {
    Node const* const* asts;
    size_t count_ast;
    Map const* str_label_map;
};
typedef struct code Code;


#ifndef SELFHOST_9MM
// main.c
void error_at(char const*, char const*);
void log_internal(char const*, const char*, const char*, size_t, char const*, ...);
char* read_file(char const*);

#define log_base(level, ...) \
    log_internal(level, __FILE__, __func__, __LINE__, __VA_ARGS__);

#define error(...)                                         \
    {                                                      \
        log_base("\033[1;31m[ERROR]\033[0m", __VA_ARGS__); \
        exit(1);                                           \
    }

#define error_if_null(var)          \
    {                               \
        if (var == NULL) {          \
            error(#var " is NULL"); \
        }                           \
    }

#define debug(...) \
    log_base("\033[1;34m[DEBUG]\033[0m", __VA_ARGS__);

// preprocessor.c
char const* preprocess(char*, char const*);

// tokenize.c
Vector const* tokenize(char const*);

// parse.c
Code const* program(Vector const*);

// codegen.c
void generate(Code const*);

void runtest();
#endif
