#include "y3c.h"

Type *ty_int = &(Type) { TY_INT, 8 };

bool is_integer(Type *ty) {
    return ty->kind == TY_INT;
}

Type *copy_type(Type *ty) {
    Type *ret = calloc(1, sizeof(Type));
    *ret = *ty;
    return ret;
}

Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->size = 8;
    ty->base = base;
    return ty;
}

Type *func_type(Type *return_ty) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_FUNC;
    ty->return_ty = return_ty;
    return ty;
}

Type *array_of(Type *base, int len) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_ARRAY;
    ty->size = base->size * len;
    ty->base = base;
    ty->array_length = len;
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
    for (Node *n = node->args; n; n = n->next) {
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
    case NODE_FUNCTION_CALL:
        node->ty = ty_int;
        return;
    case NODE_VAR:
        node->ty = node->var->ty;
        return;
    case NODE_ADDRESS:
        if (node->lhs->ty->kind == TY_ARRAY) {
            node->ty = pointer_to(node->lhs->ty->base);
        }
        else {
            node->ty = pointer_to(node->lhs->ty);
        }
        return;
    case NODE_DEREFERENCE:
        if (!node->lhs->ty->base) {
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