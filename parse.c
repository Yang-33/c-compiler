#include "9cc.h"

static Node *create_new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *create_new_binary_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = create_new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *create_new_num_node(int val) {
    Node *node = create_new_node(NODE_NUM);
    node->val = val;
    return node;
}

// Ensures that the current token is |TOKEN_NUM|.
static int take_number(Token *tok) {
    if (tok->kind != TOKEN_NUM)
        error_tok(tok, "expected a number.");
    return tok->val;
}

static Node *expr(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

// expr = equality
static Node *expr(Token **rest, Token *tok) {
    return equality(rest, tok);
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);

    for (;;) {
        if (equal(tok, "==")) {
            Node *rhs = relational(&tok, tok->next);
            node = create_new_binary_node(NODE_EQ, node, rhs);
        }
        else if (equal(tok, "!=")) {
            Node *rhs = relational(&tok, tok->next);
            node = create_new_binary_node(NODE_NE, node, rhs);
        }
        else {
            *rest = tok;
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);

    for (;;) {
        if (equal(tok, "<")) {
            Node *rhs = add(&tok, tok->next);
            node = create_new_binary_node(NODE_LT, node, rhs);
        }
        else if (equal(tok, "<=")) {
            Node *rhs = add(&tok, tok->next);
            node = create_new_binary_node(NODE_LE, node, rhs);
        }
        else if (equal(tok, ">")) {
            Node *rhs = add(&tok, tok->next);
            node = create_new_binary_node(NODE_GT, node, rhs);
        }
        else if (equal(tok, ">=")) {
            Node *rhs = add(&tok, tok->next);
            node = create_new_binary_node(NODE_GE, node, rhs);
        }
        else {
            *rest = tok;
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);

    for (;;) {
        if (equal(tok, "+")) {
            Node *rhs = mul(&tok, tok->next);
            node = create_new_binary_node(NODE_ADD, node, rhs);
        }
        else if (equal(tok, "-")) {
            Node *rhs = mul(&tok, tok->next);
            node = create_new_binary_node(NODE_SUB, node, rhs);
        }
        else {
            *rest = tok;
            return node;
        }
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tok) {
    Node *node = unary(&tok, tok);

    for (;;) {
        if (equal(tok, "*")) {
            Node *rhs = unary(&tok, tok->next);
            node = create_new_binary_node(NODE_MUL, node, rhs);
        }
        else if (equal(tok, "/")) {
            Node *rhs = unary(&tok, tok->next);
            node = create_new_binary_node(NODE_DIV, node, rhs);
        }
        else {
            *rest = tok;
            return node;
        }
    }
}

// unary = ("+" | "-") unary
//       | primary
static Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+")) {
        return unary(rest, tok->next);
    }
    else if (equal(tok, "-")) {
        return create_new_binary_node(
            NODE_SUB, create_new_num_node(0), unary(rest, tok->next));
    }
    return primary(rest, tok);
}

// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    Node *node = create_new_num_node(take_number(tok));
    *rest = tok->next;
    return node;
}

Node *parse(Token *tok) {
    Node *node = expr(&tok, tok);
    if (tok->kind != TOKEN_EOF) {
        error_tok(tok, "extra token");
    }
    return node;
}