#include "9cc.h"

static int top;
static char *reg(int idx) {
    static char *r[] = { "r10", "r11", "r12", "r13", "r14", "r15" };
    if (idx < 0 || sizeof(r) / sizeof(*r) <= (unsigned int)idx)
        error("register out of range: %d.", idx);
    return r[idx];
}


static void generate_asm(Node *node) {
    if (node->kind == NODE_NUM) {
        printf("  mov %s, %d\n", reg(top++), node->val);
        return;
    }

    generate_asm(node->lhs);
    generate_asm(node->rhs);

    char *r_lhs = reg(top - 2);
    char *r_rhs = reg(top - 1);
    top--;

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
    case NODE_SEMICOLON:
    case NODE_RETURN:
    case NODE_NUM:
        error("Internal error: invalid node. kind:= %d", node->kind);
        break;
        break;
    }
}

static void generate_statement(Node *node) {
    if (node->kind == NODE_SEMICOLON) {
        generate_asm(node->lhs);
        top--;
    }
    else if (node->kind == NODE_RETURN) {
        generate_asm(node->lhs);
        // RAX represents program exit code.
        printf("  mov rax, %s\n", reg(--top));
        printf("  jmp .L.return\n");
        return;
    }
    else {
        error("invalid statement");
    }
}


void codegen(Node *node) {
    // Print out the first half of assembly.
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // Save callee-saved registers.
    printf("  push r12\n");
    printf("  push r13\n");
    printf("  push r14\n");
    printf("  push r15\n");

    // Traverse the AST to emit assembly.
    for (Node *n = node; n; n = n->next) {
        generate_statement(n);
        assert(top == 0);
    }

    printf(".L.return:\n");
    printf("  pop r15\n");
    printf("  pop r14\n");
    printf("  pop r13\n");
    printf("  pop r12\n");
    printf("  ret\n");
}