#include "9mm.h"

static Context const* codegen_context;

#ifndef SELFHOST_9MM
static void gen(Node const*);
static void gen_loading_value(Node const*);
static void gen_var_addr(Node const*);
#endif

void generate(Code const* code)
{
    puts(".intel_syntax noprefix");
    puts(".global main\n");

    Vector const* keys = code->str_label_map->keys;
    if (keys->len != 0) {
        // Define string literals.
        puts(".data");
        puts(".align 8");
        for (size_t i = keys->len; 0 < i; i--) {
            char const* str = code->str_label_map->keys->data[i - 1];
            char const* label = code->str_label_map->vals->data[i - 1];
            printf("%s:\n", label);
            printf("  .string %s\n", str);
        }
        putchar('\n');
    }

    // Allocate the global variable spaces.
    puts("# Global variables");
    puts(".bss");
    puts(".align 32");
    Node const* const* asts = code->asts;
    for (size_t i = 0; i < code->count_ast; ++i) {
        if (asts[i]->ty == ND_GVAR_NEW) {
            printf("%s:\n", asts[i]->name);
            printf("  .zero %zd\n", asts[i]->rtype->size);
        }
    }
    putchar('\n');
    puts(".text");

    for (size_t i = 0; i < code->count_ast; ++i) {
        gen(asts[i]);
    }
}

static void gen(Node const* node)
{
    char* regs64[6];
    regs64[0] = "rdi";
    regs64[1] = "rsi";
    regs64[2] = "rdx";
    regs64[3] = "rcx";
    regs64[4] = "r8";
    regs64[5] = "r9";

    char* regs32[6];
    regs32[0] = "edi";
    regs32[1] = "esi";
    regs32[2] = "edx";
    regs32[3] = "ecx";
    regs32[4] = "r8d";
    regs32[5] = "r9d";

    if (node->ty == '!') {
        gen(node->lhs);
        printf("  xor rdx, rdx\n");
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  sete dl\n");
        printf("  push rdx\n");

        return;
    }

    if (node->ty == ND_AND) {
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .false_%p\n", node);
        gen(node->rhs);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  jne .true_%p\n", node);
        printf(".false_%p:\n", node);
        printf("  push 0\n");
        printf("  jmp .end_and_%p\n", node);
        printf(".true_%p:\n", node);
        printf("  push 1\n");
        printf(".end_and_%p:\n", node);

        return;
    }

    if (node->ty == ND_OR) {
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  jne .true_%p\n", node);
        gen(node->rhs);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  jne .true_%p\n", node);
        printf(".false_%p:\n", node);
        printf("  push 0\n");
        printf("  jmp .end_or_%p\n", node);
        printf(".true_%p:\n", node);
        printf("  push 1\n");
        printf(".end_or_%p:\n", node);

        return;
    }

    if (node->ty == ND_INCL_POST || node->ty == ND_DECL_POST) {
        // Load value from variable.
        gen(node->lhs);
        // Keep the loaded value on the stack and update the variable.
        gen(node->rhs);
        // Discard the result of update.
        printf("  pop rax\n");
        return;
    }

    if (node->ty == ND_STR) {
        printf("  lea rax, %s\n", node->label);
        printf("  push rax\n");
        return;
    }

    if (node->ty == ND_GVAR_NEW) {
        return;
    }

    if (node->ty == ND_REF) {
        gen_var_addr(node->lhs);
        return;
    }

    if (node->ty == ND_DEREF) {
        gen(node->lhs);
        gen_loading_value(node);
        return;
    }

    if (node->ty == ND_FUNCTION) {
        if (node->lhs == NULL) {
            // Skip protorype.
            // FIXME: do not include this node in AST.
            return;
        }

        printf("\n%s:\n", node->function->name);

        codegen_context = node->function->context;

        // Prorogue.
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");

        // Store arguments into the local stack.
        // FIXME: keep this size in the context.
        size_t used_size = 0;
        for (size_t i = 0; i < node->function->args->len; i++) {
            Node* arg = node->function->args->data[i];

            if (arg->rtype->size == 1) {
                printf("  mov rax, %s\n", regs64[i]);
                printf("  sub rsp, 1\n");
                printf("  mov [rsp], al\n");
                used_size += 1;
            } else if (arg->rtype->size == 4) {
                printf("  sub rsp, 4\n");
                printf("  mov [rsp], %s\n", regs32[i]);
                used_size += 4;
            } else if (arg->rtype->size == 8) {
                printf("  push %s\n", regs64[i]);
                used_size += 8;
            } else {
                error("Not supported");
            }
        }

        // Allocate the local variable space without argumens.
        printf("  sub rsp, %zd\n", codegen_context->current_offset - used_size);

        gen(node->lhs);

        // Epilogue.
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");

        codegen_context = NULL;

        return;
    }

    if (node->ty == ND_CALL) {
        Vector* args = node->call->arguments;
        if (6 < args->len) {
            error("The number of arguments has to be less than 6");
        }

        for (size_t i = 0; i < args->len; i++) {
            gen(args->data[i]);
        }

        printf("  # call %s\n", node->call->name);
        for (size_t i = 0; i < args->len; i++) {
            printf("  pop %s\n", regs64[args->len - 1 - i]);
        }

        // Align rsp with 16bytes.
        printf("  mov rax, rsp\n");
        printf("  and rsp, -8\n");
        printf("  push rax\n");
        printf("  xor al, al\n"); // for variadic function call.
        printf("  call %s\n", node->call->name);
        printf("  pop rsp\n");
        printf("  push rax\n");

        return;
    }

    if (node->ty == ND_BLOCK) {
        for (size_t i = 0; i < node->stmts->len; i++) {
            gen(node->stmts->data[i]);
            printf("  pop rax\n");
        }
        // The last one is used the result.
        printf("  push rax\n");
        return;
    }

    if (node->ty == ND_IF) {
        gen(node->if_else->condition);

        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .L_else_%p\n", node->if_else);

        gen(node->if_else->body);

        if (node->if_else->else_body == NULL) {
            printf("  .L_else_%p:\n", node->if_else);
            // Push dummy value.
            printf("  push 0\n");
        } else {
            printf("  jmp .L_if_end_%p\n", node->if_else);
            printf("  .L_else_%p:\n", node->if_else);
            gen(node->if_else->else_body);
            printf("  .L_if_end_%p:\n", node->if_else);
        }

        return;
    }

    if (node->ty == ND_BREAK) {
        // Push dummy value.
        printf("  push 0\n");
        printf("  jmp %s\n", node->break_label);

        return;
    }

    if (node->ty == ND_WHILE) {
        printf("  .L_while_begin_%p:\n", node);

        gen(node->lhs);

        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je %s\n", node->break_label);

        gen(node->rhs);
        printf("  jmp .L_while_begin_%p\n", node);

        printf("  %s:\n", node->break_label);

        return;
    }

    if (node->ty == ND_FOR) {
        if (node->fors->initializing != NULL) {
            gen(node->fors->initializing);
        }
        printf("  .L_for_begin_%p:\n", node->fors);

        if (node->fors->condition != NULL) {
            gen(node->fors->condition);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je %s\n", node->fors->break_label);
        }

        gen(node->fors->body);
        if (node->fors->updating != NULL) {
            gen(node->fors->updating);
        }

        printf("  jmp .L_for_begin_%p\n", node->fors);

        printf("  %s:\n", node->fors->break_label);

        return;
    }

    if (node->ty == ND_RETURN) {
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
        return;
    }

    if (node->ty == ND_NUM) {
        printf("  mov rax, %zd\n", node->val);
        printf("  push rax\n");
        return;
    }

    if (node->ty == ND_LVAR_NEW) {
        // Push a dummy value for pop after that because it's based on stack machine.
        printf("  push 0\n");
        return;
    }

    if (node->ty == ND_LVAR || node->ty == ND_GVAR || node->ty == ND_DOT_REF || node->ty == ND_ARROW_REF) {
        gen_var_addr(node);

        error_if_null(node->rtype);

        if (node->rtype->ty != ARRAY) {
            gen_loading_value(node);
        }

        return;
    }

    if (node->ty == ND_INIT) {
        gen(node->lhs);
        printf("  pop rax\n");
        gen(node->rhs);
        return;
    }

    if (node->ty == '=') {
        // Assignment.
        gen_var_addr(node->lhs);
        gen(node->rhs);

        printf("  # Assignment\n");
        printf("  pop rdx\n");
        printf("  pop rax\n");

        error_if_null(node->rtype);
        if (node->rtype->size == 1) {
            printf("  mov [rax], dl\n");
        } else if (node->rtype->size == 4) {
            printf("  mov [rax], edx\n");
        } else if (node->rtype->size == 8) {
            printf("  mov [rax], rdx\n");
        } else {
            error("Not supported");
        }

        printf("  push rdx\n");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    int ty = node->ty;

    if (ty == ND_EQ) {
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
    } else if (ty == ND_NE) {
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
    } else if (ty == '<') {
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
    } else if (ty == TK_LE) {
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
    } else if (ty == TK_GE || ty == '>') {
        error("parser has a bug");
    } else if (ty == '+') {
        printf("  add rax, rdi\n");
    } else if (ty == '-') {
        printf("  sub rax, rdi\n");
    } else if (ty == '*') {
        // rax * rdi
        // rdx has upper bits, rax has lower bits.
        printf("  imul rdi\n");
    } else if (ty == '/') {
        // cqo instruction expands value in rax 128 and set rdx and rax.
        printf("  cqo\n");
        printf("  idiv rdi\n");
    }

    printf("  push rax\n");
}

// Push the address of the given variable on the stack top.
static void gen_var_addr(Node const* node)
{
    if (node->ty == ND_LVAR) {
        printf("  # Reference local var: %s\n", node->name);
        printf("  mov rax, rbp\n");
        printf("  sub rax, %zd\n", (size_t)map_get(codegen_context->var_offset_map, node->name));
        printf("  push rax\n");
    } else if (node->ty == ND_GVAR) {
        printf("  # Reference global var: %s\n", node->name);
        printf("  lea rax, %s\n", node->name);
        printf("  push rax\n");
    } else if (node->ty == ND_DEREF || node->ty == ND_STR) {
        gen(node->lhs);
    } else if (node->ty == ND_DOT_REF || node->ty == ND_ARROW_REF) {
        gen_var_addr(node->lhs);
        printf("  pop rax\n");
        printf("  add rax, %zd\n", node->member_offset);
        printf("  push rax\n");
    } else {
        error("You can only get address of variable");
    }
}

// Load the value which is pointed by the address on the stacktop.
static void gen_loading_value(Node const* node)
{
    error_if_null(node);
    error_if_null(node->rtype);

    printf("  pop rax\n");

    if (node->rtype->size == 1) {
        printf("  movzx rax, BYTE PTR [rax]\n");
    } else if (node->rtype->size == 4) {
        printf("  mov eax, [rax]\n");
    } else if (node->rtype->size == 8) {
        printf("  mov rax, [rax]\n");
    } else {
        error("Not supported");
    }

    printf("  push rax\n");
}
