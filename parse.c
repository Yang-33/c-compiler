#include "9cc.h"

// All local variable instances created during parsing are accumulated to this
// list.
Var *locals;

static Node *multi_statement(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

char *mystrndup(const char *s, size_t n) {
    char *new = (char *)malloc(n + 1);
    if (new == NULL) {
        return NULL;
    }
    new[n] = '\0';
    return (char *)memcpy(new, s, n);
}

static Var *find_var(Token *tok) {
    for (Var *var = locals; var; var = var->next) {
        if (strlen(var->name) == tok->token_length
            && !strncmp(tok->token_string, var->name, tok->token_length)) {
            return var;
        }
    }
    return NULL;
}

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

static Node *create_new_unary_node(NodeKind kind, Node *lhs) {
    Node *node = create_new_node(kind);
    node->lhs = lhs;
    return node;
}

static Node *create_new_num_node(int val) {
    Node *node = create_new_node(NODE_NUM);
    node->val = val;
    return node;
}

static Node *create_new_var_node(Var *var) {
    Node *node = create_new_node(NODE_VAR);
    node->var = var;
    return node;
}

static Var *create_new_local_var(char *name) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->next = locals;
    locals = var;
    // offset will be set after all tokens are parsed.
    return  var;
}

// Ensures that the current token is |TOKEN_NUM|.
static int take_number(Token *tok) {
    if (tok->kind != TOKEN_NUM)
        error_tok(tok, "expected a number.");
    return tok->val;
}

// statement = "return" expr ";"
//           | "if" "(" expr ")" statement ("else" statement)?
//           | "for" "(" expr? ";" expr? ";" expr? ")" statement
//           | "while" "(" expr ")" statement
//           | "{" multi_statement "}"
//           | expr ";"
static Node *statement(Token **rest, Token *tok) {
    if (equal(tok, "return")) {
        Node *node = create_new_unary_node(NODE_RETURN, expr(&tok, tok->next));
        *rest = skip(tok, ";");
        return node;
    }
    if (equal(tok, "if")) {
        Node *node = create_new_node(NODE_IF);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = statement(&tok, tok);
        if (equal(tok, "else")) {
            node->els = statement(&tok, tok->next);
        }
        *rest = tok;
        return node;
    }

    if (equal(tok, "for")) {
        Node *node = create_new_node(NODE_FOR);
        tok = skip(tok->next, "(");

        if (!equal(tok, ";")) {
            node->init =
                create_new_unary_node(NODE_EXPR_STATEMENT, expr(&tok, tok));
        }
        tok = skip(tok, ";");

        if (!equal(tok, ";")) {
            node->cond = expr(&tok, tok);
        }
        tok = skip(tok, ";");

        if (!equal(tok, ")")) {
            node->inc =
                create_new_unary_node(NODE_EXPR_STATEMENT, expr(&tok, tok));
        }
        tok = skip(tok, ")");

        node->then = statement(&tok, tok);
        *rest = tok;
        return node;
    }

    if (equal(tok, "while")) {
        Node *node = create_new_node(NODE_FOR);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = statement(rest, tok);
        return node;
    }

    if (equal(tok, "{")) {
        Node *node = multi_statement(&tok, tok->next);
        tok = skip(tok, "}");
        *rest = tok;
        return node;
    }

    Node *node = create_new_unary_node(NODE_EXPR_STATEMENT, expr(&tok, tok));
    *rest = skip(tok, ";");
    return node;
}

// multi_statement = statement*
static Node *multi_statement(Token **rest, Token *tok) {
    Node head;
    Node *tail = &head;
    while (!equal(tok, "}")) {
        tail = tail->next = statement(&tok, tok);
    }

    Node *node = create_new_node(NODE_BLOCK);
    node->body = head.next;
    *rest = tok;
    return node;
}

// expr = assign
static Node *expr(Token **rest, Token *tok) {
    return assign(rest, tok);
}

// assign = equality ("=" assign)?
static Node *assign(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    if (equal(tok, "=")) {
        node = create_new_binary_node(NODE_ASSIGN, node, assign(&tok, tok->next));
    }
    *rest = tok;
    return node;
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

// primary = "(" expr ")" | num | idnetifier
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    if (tok->kind == TOKEN_IDENTIFIER) {
        Var *var = find_var(tok);
        if (!var) {
            var = create_new_local_var(
                mystrndup(tok->token_string, tok->token_length));
        }
        *rest = tok->next;
        return create_new_var_node(var);
    }
    Node *node = create_new_num_node(take_number(tok));
    *rest = tok->next;
    return node;
}


// program = "{" multi_statement "}"
Function *parse(Token *tok) {
    tok = skip(tok, "{");
    Function *prog = calloc(1, sizeof(Function));
    prog->node = multi_statement(&tok, tok)->body;
    tok = skip(tok, "}");

    prog->locals = locals;
    // Function.stack_size will be set after all tokens are parsed.
    return prog;
}
