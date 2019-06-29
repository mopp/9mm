#include "9mm.h"
#include <stdio.h>

static char const* const regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static Context const* context = NULL;

static void gen_lval(Node const*);

void gen(Node const* node)
{
    if (node->ty == ND_REF) {
        if (node->lhs->ty != ND_LVAR) {
            error("変数以外のアドレスは取得できません");
        }
        gen_lval(node->lhs);
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
        for (size_t i = 0; i < context->count_vars; i++) {
            printf("  push %s\n", regs[i]);
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
            printf("  pop %s\n", regs[args->len - 1 - i]);
        }

        // rspを16バイトアラインメントにする.
        // 8バイトでalignして古いrspを格納する.
        printf("  mov rax, rsp\n");
        printf("  and rsp, -8\n");
        printf("  push rax\n");
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
        printf("  # Create local var\n");
        printf("  sub rsp, 8\n");
        // スタック調節のために空の値を入れておく.
        printf("  push 0\n");
        return;
    }

    if (node->ty == ND_LVAR) {
        // 変数の値をraxにロードする.
        gen_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }

    if (node->ty == '=') {
        // `a = 1`に対応する.
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
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


// 与えられたノードの持つ変数のアドレスをスタックにpushする
static void gen_lval(Node const* node)
{
    if (node->ty != ND_LVAR) {
        error("代入の左辺値が変数ではありません");
    }

    printf("  # Reference local var\n");
    printf("  mov rax, rbp\n");
    printf("  sub rax, %zd\n", (uintptr_t)map_get(context->var_offset_map, node->name));
    printf("  push rax\n");
}
