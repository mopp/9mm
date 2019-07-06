#include "9mm.h"
#include <stdbool.h>
#include <stdio.h>

static char const* const regs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static char const* const regs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};

static Context const* context = NULL;

static void gen_var_addr(Node const*);

void gen(Node const* node)
{
    if (node->ty == ND_STR) {
        printf("  lea rax, %s\n", node->label);
        printf("  push rax\n");
        return;
    }

    if (node->ty == ND_GVAR_NEW) {
        return;
    }

    if (node->ty == ND_REF) {
        Node* n = node->lhs;
        if (n->ty == ND_GVAR || n->ty == ND_LVAR) {
            gen_var_addr(node->lhs);
            return;
        } else if (n->ty == ND_DEREF && n->lhs->ty == '+' && n->lhs->lhs->rtype->ty == ARRAY) {
            // Get address of an element of an array.
            // e.g., &a[0]
            gen(n->lhs);
            return;
        }

        error("You cannot get address: %zd, %zd", n->ty, n->rtype->ty);
        return;
    }

    if (node->ty == ND_DEREF) {
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  push [rax]\n");
        return;
    }

    if (node->ty == ND_FUNCTION) {
        printf("%s:\n", node->function->name);

        context = node->function->context;

        // プロローグ.
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");

        // 引数をスタックへ書き出し.
        for (size_t i = 0; i < node->function->args->len; i++) {
            Node* arg = node->function->args->data[i];
            if (arg->rtype->size == 4) {
                printf("  sub rsp, 4\n");
                printf("  mov [rsp], %s\n", regs32[i]);
            } else {
                printf("  push %s\n", regs64[i]);
            }
        }

        // 本体のblockを展開.
        // 結果はraxに格納済み.
        gen(node->lhs);

        // エピローグ.
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");

        context = NULL;

        return;
    }

    if (node->ty == ND_CALL) {
        Vector* args = node->call->arguments;
        if (6 < args->len) {
            error("6個より多い引数には対応していない");
        }

        for (size_t i = 0; i < args->len; i++) {
            gen(args->data[i]);
        }

        printf("  # call %s\n", node->call->name);
        for (size_t i = 0; i < args->len; i++) {
            printf("  pop %s\n", regs64[args->len - 1 - i]);
        }

        // rspを16バイトアラインメントにする.
        // 8バイトでalignして古いrspを格納する.
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
        // 最後のものを結果として扱う.
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
            // 空の結果を入れておく.
            printf("  push 0\n");
        } else {
            printf("  jmp .L_if_end_%p\n", node->if_else);
            printf("  .L_else_%p:\n", node->if_else);
            gen(node->if_else->else_body);
            printf("  .L_if_end_%p:\n", node->if_else);
        }

        return;
    }

    if (node->ty == ND_WHILE) {
        printf("  .L_while_begin_%p:\n", node);

        gen(node->lhs);

        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .L_while_end_%p\n", node);

        gen(node->rhs);
        printf("  jmp .L_while_begin_%p\n", node);

        printf("  .L_while_end_%p:\n", node);

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
            printf("  je .L_for_end_%p\n", node->fors);
        }

        gen(node->fors->body);
        if (node->fors->updating != NULL) {
            gen(node->fors->updating);
        }

        printf("  jmp .L_for_begin_%p\n", node->fors);

        printf("  .L_for_end_%p:\n", node->fors);

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
        printf("  push %d\n", node->val);
        return;
    }

    if (node->ty == ND_LVAR_NEW) {
        printf("  sub rsp, %zd\n", node->rtype->size);
        // Push a dummy value for pop after that because it's based on stack machine.
        printf("  push 0\n");
        return;
    }

    if (node->ty == ND_LVAR || node->ty == ND_GVAR) {
        // 変数の値をraxにロードする.
        gen_var_addr(node);

        if (node->rtype->ty != ARRAY) {
            printf("  pop rax\n");
            if (node->rtype->size == 1) {
                printf("  movsx ecx, BYTE PTR [rax]\n");
            } else if (node->rtype->size == 4) {
                printf("  mov eax, [rax]\n");
            } else {
                printf("  mov rax, [rax]\n");
            }
            printf("  push rax\n");
        }

        return;
    }

    if (node->ty == '=') {
        // `a = 1`に対応する.
        if (node->lhs->ty == ND_DEREF || node->lhs->ty == ND_STR) {
            gen(node->lhs->lhs);
        } else {
            gen_var_addr(node->lhs);
        }
        gen(node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");

        if (node->rtype->size == 1) {
            printf("  mov [rax], dl\n");
        } else if (node->rtype->size == 4) {
            printf("  mov [rax], edi\n");
        } else {
            printf("  mov [rax], rdi\n");
        }

        printf("  push rdi\n");
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


// Push the address of the given variable on the stack top.
static void gen_var_addr(Node const* node)
{
    if (node->ty == ND_LVAR) {
        printf("  # Reference local var\n");
        printf("  mov rax, rbp\n");
        printf("  sub rax, %zd\n", (uintptr_t)map_get(context->var_offset_map, node->name));
        printf("  push rax\n");
    } else if (node->ty == ND_GVAR) {
        printf("  lea rax, %s\n", node->name);
        printf("  push rax\n");
    } else {
        error("You can only get address of variable");
    }
}
