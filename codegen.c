#include "9cc.h"

static int top;
static int labelseq = 1;
static char *reg(int idx) {
    static char *r[] = { "r10", "r11", "r12", "r13", "r14", "r15" };
    if (idx < 0 || sizeof(r) / sizeof(*r) <= (unsigned int)idx)
        error("register out of range: %d.", idx);
    return r[idx];
}

static void generate_asm(Node *node);

// Pushes the given node's address to the stack.
static void generate_address(Node *node) {
    if (node->kind == NODE_VAR) {
        printf("  lea %s, [rbp-%d]\n", reg(top++), node->var->offset);
        return;
    }
    else if (node->kind == NODE_DEREFERENCE) {
        generate_asm(node->lhs);
        return;
    }

    error("%s is not an lvalue", node->tok->token_string);
}

static void load(void) {
    printf("  mov %s, [%s]\n", reg(top - 1), reg(top - 1));
}

static void store(void) {
    printf("  mov [%s], %s\n", reg(top - 1), reg(top - 2));
    --top;
}

static void generate_asm(Node *node) {
    if (node->kind == NODE_NUM) {
        printf("  mov %s, %d\n", reg(top++), node->val);
        return;
    }
    else if (node->kind == NODE_VAR) {
        generate_address(node);
        load();
        return;
    }
    else if (node->kind == NODE_ADDRESS) {
        generate_address(node->lhs);
        return;
    }
    else if (node->kind == NODE_DEREFERENCE) {
        generate_asm(node->lhs);
        load();
        return;
    }
    else if (node->kind == NODE_ASSIGN) {
        generate_asm(node->rhs);
        generate_address(node->lhs);
        store();
        return;
    }


    generate_asm(node->lhs);
    generate_asm(node->rhs);

    char *r_lhs = reg(top - 2);
    char *r_rhs = reg(top - 1);
    --top;

    switch (node->kind) {
    case NODE_ADD:
        printf("  add %s, %s\n", r_lhs, r_rhs);
        break;
    case NODE_SUB:
        printf("  sub %s, %s\n", r_lhs, r_rhs);
        break;
    case NODE_MUL:
        printf("  imul %s, %s\n", r_lhs, r_rhs);
        break;
    case NODE_DIV:
        printf("  mov rax, %s\n", r_lhs);
        // RDX:RAX <- sign-extend of RAX.
        printf("  cqo\n");
        // Signed divide RDX:RAX by rdi with result stored in RAX(quotient),
        // RDX(remainder)
        printf("  idiv %s\n", r_rhs);
        printf("  mov %s, rax\n", r_lhs);
        break;
    case NODE_EQ:
        printf("  cmp %s, %s\n", r_lhs, r_rhs);
        printf("  sete al\n");
        printf("  movzx %s, al\n", r_lhs);
        break;
    case NODE_NE:
        printf("  cmp %s, %s\n", r_lhs, r_rhs);
        printf("  setne al\n");
        printf("  movzx %s, al\n", r_lhs);
        break;
    case NODE_LT:
        printf("  cmp %s, %s\n", r_lhs, r_rhs);
        printf("  setl al\n");
        printf("  movzx %s, al\n", r_lhs);
        break;
    case NODE_LE:
        printf("  cmp %s, %s\n", r_lhs, r_rhs);
        printf("  setle al\n");
        printf("  movzx %s, al\n", r_lhs);
        break;
    case NODE_GT:
        printf("  cmp %s, %s\n", r_lhs, r_rhs);
        printf("  setg al\n");
        printf("  movzx %s, al\n", r_lhs);
        break;
    case NODE_GE:
        printf("  cmp %s, %s\n", r_lhs, r_rhs);
        printf("  setge al\n");
        printf("  movzx %s, al\n", r_lhs);
        break;
    case NODE_ADDRESS:
    case NODE_DEREFERENCE:
    case NODE_EXPR_STATEMENT:
    case NODE_RETURN:
    case NODE_ASSIGN:
    case NODE_IF:
    case NODE_FOR:
    case NODE_BLOCK:
    case NODE_VAR:
    case NODE_NUM:
        error("Internal error: invalid node. kind:= %d, token:= %s",
            node->kind, node->tok->token_string);
        break;
    }
}

static void generate_statement(Node *node) {
    if (node->kind == NODE_EXPR_STATEMENT) {
        generate_asm(node->lhs);
        --top;
    }
    else if (node->kind == NODE_RETURN) {
        generate_asm(node->lhs);
        // RAX represents program exit code.
        printf("  mov rax, %s\n", reg(--top));
        printf("  jmp .L.return\n");
        return;
    }
    else if (node->kind == NODE_IF) {
        int seq = labelseq++;
        if (node->els) {
            generate_asm(node->cond);
            printf("  cmp %s, 0\n", reg(--top));
            printf("  je   .L.else.%d\n", seq);
            generate_statement(node->then);
            printf("  jmp  .L.end.%d\n", seq);
            printf(".L.else.%d:\n", seq);
            generate_statement(node->els);
            printf(".L.end.%d:\n", seq);
        }
        else {
            generate_asm(node->cond);
            printf("  cmp %s, 0\n", reg(--top));
            printf("  je   .L.end.%d\n", seq);
            generate_statement(node->then);
            printf(".L.end.%d:\n", seq);
        }
    }
    else if (node->kind == NODE_FOR) {
        int seq = labelseq++;
        if (node->init) {
            generate_statement(node->init);
        }
        printf(".L.begin.%d:\n", seq);
        if (node->cond) {
            generate_asm(node->cond);
            printf("  cmp %s, 0\n", reg(--top));
            printf("  je  .L.end.%d\n", seq);
        }
        generate_statement(node->then);
        if (node->inc) {
            generate_statement(node->inc);
        }
        printf("  jmp .L.begin.%d\n", seq);
        printf(".L.end.%d:\n", seq);
    }
    else if (node->kind == NODE_BLOCK) {
        for (Node *n = node->body; n; n = n->next) {
            generate_statement(n);
        }
    }
    else {
        error("%s is invalid statement", node->tok->token_string);
    }
}


void codegen(Function *prog) {
    // Print out the first half of assembly.
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // Prologue. r12-15 are callee-saved registers.
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", prog->stack_size);
    printf("  mov [rbp-8], r12\n");
    printf("  mov [rbp-16], r13\n");
    printf("  mov [rbp-24], r14\n");
    printf("  mov [rbp-32], r15\n");

    // Traverse the AST to emit assembly.
    for (Node *n = prog->node; n; n = n->next) {
        generate_statement(n);
        assert(top == 0);
    }

    // Epilogue
    printf(".L.return:\n");
    printf("  mov r12, [rbp-8]\n");
    printf("  mov r13, [rbp-16]\n");
    printf("  mov r14, [rbp-24]\n");
    printf("  mov r15, [rbp-32]\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}