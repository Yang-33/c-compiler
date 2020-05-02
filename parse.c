#include "9cc.h"

// All local variable instances created during parsing are accumulated to this
// list.
Var *locals;

static Node *declaration(Token **rest, Token *tok);
static Node *multi_statement(Token **rest, Token *tok);
static Node *statement(Token **rest, Token *tok);
static Node *expr_statement(Token **rest, Token *tok);
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

static Node *create_new_node(NodeKind kind, Token *tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *create_new_binary_node(
    NodeKind kind, Node *lhs, Node *rhs, Token *tok) {

    Node *node = create_new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *create_new_unary_node(NodeKind kind, Node *lhs, Token *tok) {
    Node *node = create_new_node(kind, tok);
    node->lhs = lhs;
    return node;
}

static Node *create_new_num_node(int val, Token *tok) {
    Node *node = create_new_node(NODE_NUM, tok);
    node->val = val;
    return node;
}

static Node *create_new_var_node(Var *var, Token *tok) {
    Node *node = create_new_node(NODE_VAR, tok);
    node->var = var;
    return node;
}

static Var *create_new_local_var(char *name, Type *ty) {
    Var *var = calloc(1, sizeof(Var));
    var->name = name;
    var->ty = ty;
    var->next = locals;
    locals = var;
    // offset will be set after all tokens are parsed.
    return  var;
}

static char *get_identifier(Token *tok) {
    if (tok->kind != TOKEN_IDENTIFIER) {
        error_tok(tok, "expected an identifier.");
    }
    return mystrndup(tok->token_string, tok->token_length);
}

// Ensures that the current token is |TOKEN_NUM|.
static int take_number(Token *tok) {
    if (tok->kind != TOKEN_NUM)
        error_tok(tok, "expected a number.");
    return tok->val;
}

// typespec = "int"
static Type *typespec(Token **rest, Token *tok) {
    *rest = skip(tok, "int");
    return ty_int;
}

// declarator = "*" * identifier
static Type *declarator(Token **rest, Token *tok, Type *ty) {
    while (consume(&tok, tok, "*")) {
        ty = pointer_to(ty);
    }
    if (tok->kind != TOKEN_IDENTIFIER) {
        error_tok(tok, "expected a variable name.");
    }

    ty->name = tok;
    *rest = tok->next;
    return ty;
}

// declaration = typespec (declarator ("=" expr)?
//                    ("," declarator ("=" expr)? )* )? ";"
static Node *declaration(Token **rest, Token *tok) {
    Type *basety = typespec(&tok, tok);

    Node head;
    head.next = NULL;
    Node *tail = &head;
    int cnt = 0;
    while (!equal(tok, ";")) {
        if (cnt++ > 0) {
            tok = skip(tok, ",");
        }
        Type *ty = declarator(&tok, tok, basety);
        Var *var = create_new_local_var(get_identifier(ty->name), ty);

        if (!equal(tok, "=")) {
            continue;
        }
        Node *lhs = create_new_var_node(var, ty->name);
        tok = skip(tok, "=");
        Node *rhs = assign(&tok, tok);
        Node *node = create_new_binary_node(NODE_ASSIGN, lhs, rhs, tok);
        tail = tail->next = create_new_unary_node(NODE_EXPR_STATEMENT, node, tok);
    }

    Node *node = create_new_node(NODE_BLOCK, tok);
    node->body = head.next;
    tok = skip(tok, ";");
    *rest = tok;
    return node;
}
// statement = "return" expr ";"
//           | "if" "(" expr ")" statement ("else" statement)?
//           | "for" "(" expr? ";" expr? ";" expr? ")" statement
//           | "while" "(" expr ")" statement
//           | "{" multi_statement "}"
//           | expr ";"
static Node *statement(Token **rest, Token *tok) {
    if (equal(tok, "return")) {
        Node *node = create_new_node(NODE_RETURN, tok);
        node->lhs = expr(&tok, tok->next);
        *rest = skip(tok, ";");
        return node;
    }
    if (equal(tok, "if")) {
        Node *node = create_new_node(NODE_IF, tok);
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
        Node *node = create_new_node(NODE_FOR, tok);
        tok = skip(tok->next, "(");

        if (!equal(tok, ";")) {
            node->init = expr_statement(&tok, tok);
        }
        tok = skip(tok, ";");

        if (!equal(tok, ";")) {
            node->cond = expr(&tok, tok);
        }
        tok = skip(tok, ";");

        if (!equal(tok, ")")) {
            node->inc = expr_statement(&tok, tok);
        }
        tok = skip(tok, ")");

        node->then = statement(&tok, tok);
        *rest = tok;
        return node;
    }

    if (equal(tok, "while")) {
        Node *node = create_new_node(NODE_FOR, tok);
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
    Node *node = expr_statement(&tok, tok);
    *rest = skip(tok, ";");
    return node;
}

// multi_statement = (declaration | statement)*
static Node *multi_statement(Token **rest, Token *tok) {
    Node *node = create_new_node(NODE_BLOCK, tok);

    Node head;
    head.next = NULL;
    Node *tail = &head;
    while (!equal(tok, "}")) {
        if (equal(tok, "int")) {
            tail = tail->next = declaration(&tok, tok);
        }
        else {
            tail = tail->next = statement(&tok, tok);
        }
        add_type(tail);
    }

    node->body = head.next;
    *rest = tok;
    return node;
}

// expr_statement = expr
static Node *expr_statement(Token **rest, Token *tok) {
    Node *node = create_new_node(NODE_EXPR_STATEMENT, tok);
    node->lhs = expr(rest, tok);
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
        return create_new_binary_node(
            NODE_ASSIGN, node, assign(rest, tok->next), tok);
    }
    *rest = tok;
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);

    for (;;) {
        if (equal(tok, "==")) {
            node = create_new_binary_node(NODE_EQ, node, NULL, tok);
            node->rhs = relational(&tok, tok->next);
        }
        else if (equal(tok, "!=")) {
            node = create_new_binary_node(NODE_NE, node, NULL, tok);
            node->rhs = relational(&tok, tok->next);
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
            node = create_new_binary_node(NODE_LT, node, NULL, tok);
            node->rhs = add(&tok, tok->next);
        }
        else if (equal(tok, "<=")) {
            node = create_new_binary_node(NODE_LE, node, NULL, tok);
            node->rhs = add(&tok, tok->next);
        }
        else if (equal(tok, ">")) {
            node = create_new_binary_node(NODE_GT, node, NULL, tok);
            node->rhs = add(&tok, tok->next);
        }
        else if (equal(tok, ">=")) {
            node = create_new_binary_node(NODE_GE, node, NULL, tok);
            node->rhs = add(&tok, tok->next);
        }
        else {
            *rest = tok;
            return node;
        }
    }
}

// In C, '+' operator is overloaded to perform the pointer arithmetic.
// If p is a pointer, p + n adds not n but sizeof (*p) * n to the value of p,
// so that p + n points to the location n elements (not bytes) ahead of p.
// In other words, we neet to scale an integer value before adding to a pointer
// value. This function takes care of the scaling.
static Node *create_new_add_node(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // number + number
    if (is_integer(lhs->ty) && is_integer(rhs->ty)) {
        return create_new_binary_node(NODE_ADD, lhs, rhs, tok);
    }

    // pointer + pointer
    if (lhs->ty->base && rhs->ty->base) {
        error_tok(tok, " pointer + pointer is invalid operands.");
    }

    // Canonicalize 'number + pointer' to 'pointer + number'.
    if (!lhs->ty->base && rhs->ty->base) {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    rhs = create_new_binary_node(
        NODE_MUL, rhs, create_new_num_node(8, tok), tok);
    return create_new_binary_node(NODE_ADD, lhs, rhs, tok);

}

// Like '+', '-' is overloaded for the pointer type.
static Node *create_new_sub_node(Node *lhs, Node *rhs, Token *tok) {
    add_type(lhs);
    add_type(rhs);

    // number - number
    if (is_integer(lhs->ty) && is_integer(rhs->ty)) {
        return create_new_binary_node(NODE_SUB, lhs, rhs, tok);
    }

    // pointer - number
    if (lhs->ty->base && is_integer(rhs->ty)) {
        rhs = create_new_binary_node(
            NODE_MUL, rhs, create_new_num_node(8, tok), tok);
        return create_new_binary_node(NODE_SUB, lhs, rhs, tok);
    }

    // pointer - pointer
    if (lhs->ty->base && rhs->ty->base) {
        Node *node = create_new_binary_node(NODE_SUB, lhs, rhs, tok);
        return create_new_binary_node(
            NODE_DIV, node, create_new_num_node(8, tok), tok);
    }

    // number - pointer
    error_tok(tok, "number - pointer is invalid operands.");
    return create_new_num_node(0, tok);
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);

    for (;;) {
        Token *start = tok;
        if (equal(tok, "+")) {
            node = create_new_add_node(node, mul(&tok, tok->next), start);
        }
        else if (equal(tok, "-")) {
            node = create_new_sub_node(node, mul(&tok, tok->next), start);
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
            node = create_new_binary_node(NODE_MUL, node, NULL, tok);
            node->rhs = unary(&tok, tok->next);
        }
        else if (equal(tok, "/")) {
            node = create_new_binary_node(NODE_DIV, node, NULL, tok);
            node->rhs = unary(&tok, tok->next);
        }
        else {
            *rest = tok;
            return node;
        }
    }
}

// unary = ("+" | "-" | "*" | "&") unary
//       | primary
static Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+")) {
        return unary(rest, tok->next);
    }
    else if (equal(tok, "-")) {
        return create_new_binary_node(
            NODE_SUB, create_new_num_node(0, tok), unary(rest, tok->next), tok);
    }
    else if (equal(tok, "&")) {
        return create_new_unary_node(NODE_ADDRESS, unary(rest, tok->next), tok);
    }
    else if (equal(tok, "*")) {
        return create_new_unary_node(
            NODE_DEREFERENCE, unary(rest, tok->next), tok);
    }
    return primary(rest, tok);
}

// func-args = "(" (assign ("," assign)* )? ")"
static Node *func_args(Token **rest, Token *tok) {
    Node head = {};
    Node *tail = &head;

    while (!equal(tok, ")")) {
        if (tail != &head) {
            tok = skip(tok, ",");
        }
        tail = tail->next = assign(&tok, tok);
    }

    *rest = skip(tok, ")");
    return head.next;
}

// primary = "(" expr ")" | num | idnetifier func-args?
static Node *primary(Token **rest, Token *tok) {
    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    if (tok->kind == TOKEN_IDENTIFIER) {
        // Function call
        if (equal(tok->next, "(")) {
            Node *node = create_new_node(NODE_FUNCTION_CALL, tok);
            node->funcname = mystrndup(tok->token_string, tok->token_length);
            node->args = func_args(rest, tok->next->next);
            return node;
        }

        // Variable
        Var *var = find_var(tok);
        if (!var) {
            error_tok(tok, "undefined variable.");
        }
        *rest = tok->next;
        return create_new_var_node(var, tok);
    }
    Node *node = create_new_num_node(take_number(tok), tok);
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
