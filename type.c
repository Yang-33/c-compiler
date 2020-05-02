#include "9cc.h"

Type *ty_int = &(Type) { TY_INT };

bool is_integer(Type *ty) {
    return ty->kind == TY_INT;
}

Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->base = base;
    return ty;
}

void add_type(Node *node) {
    if (!node || node->ty) {
        return;
    }

    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->then);
    add_type(node->els);
    add_type(node->init);
    add_type(node->inc);

    for (Node *n = node->body; n; n = n->next) {
        add_type(n);
    }
    switch (node->kind) {
    case NODE_ADD:
    case NODE_SUB:
    case NODE_MUL:
    case NODE_DIV:
    case NODE_ASSIGN:
        node->ty = node->lhs->ty;
        return;
    case NODE_EQ:
    case NODE_NE:
    case NODE_LT:
    case NODE_LE:
    case NODE_GT:
    case NODE_GE:
    case NODE_NUM:
        node->ty = ty_int;
        return;
    case NODE_VAR:
        node->ty = node->var->ty;
        return;
    case NODE_ADDRESS:
        node->ty = pointer_to(node->lhs->ty);
        return;
    case NODE_DEREFERENCE:
        if (node->lhs->ty->kind != TY_PTR) {
            error_tok(node->tok, "invalid pointer dereference");
        }
        node->ty = node->lhs->ty->base;
        return;
    case NODE_RETURN:
    case NODE_IF:
    case NODE_FOR:
    case NODE_BLOCK:
    case NODE_EXPR_STATEMENT:
        return;
    }
}