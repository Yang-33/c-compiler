#include "9cc.h"

static void generate_asm(Node *node) {
    if (node->kind == NODE_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    generate_asm(node->lhs);
    generate_asm(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
    case NODE_ADD:
        printf("  add rax, rdi\n");
        break;
    case NODE_SUB:
        printf("  sub rax, rdi\n");
        break;
    case NODE_MUL:
        printf("  imul rax, rdi\n");
        break;
    case NODE_DIV:
        // RDX:RAX <- sign-extend of RAX.
        printf("  cqo\n");
        // Signed divide RDX:RAX by rdi with result stored in RAX(quotient),
        // RDX(remainder)
        printf("  idiv rdi\n");
        break;
    case NODE_EQ:
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
        break;
    case NODE_NE:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    case NODE_LT:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case NODE_LE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    case NODE_GT:
        printf("  cmp rax, rdi\n");
        printf("  setg al\n");
        printf("  movzb rax, al\n");
        break;
    case NODE_GE:
        printf("  cmp rax, rdi\n");
        printf("  setge al\n");
        printf("  movzb rax, al\n");
        break;
    case NODE_NUM:
        error("Internal error: invalid node. kind:= %d", node->kind);
        break;
    }

    printf("  push rax\n");
}

void codegen(Node *node) {
    // Print out the first half of assembly.
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // Traverse the AST to emit assembly.
    generate_asm(node);

    // A result must be at the top of the stack, so pop it to RAX to make it a
    // program exit code.
    printf("  pop rax\n");
    printf("  ret\n");
}