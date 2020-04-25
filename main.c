#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Tokenizer
//

typedef enum {
    TOKEN_SYMBOL,
    TOKEN_NUM,
    TOKEN_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    Token *next;
    int val; // If kind is TOKEN_NUM, it's assigned
    char *token_string;
    size_t token_length;
};

// Input string
static char *current_input;

// Reports an error and exit.
static void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Reports an error location and exit.
static void compile_error_at(char *token_string, char *fmt, va_list ap) {
    int pos = token_string - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void error_at(char *token_string, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    compile_error_at(token_string, fmt, ap);
}

static void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    compile_error_at(tok->token_string, fmt, ap);
}

// Check that the current token equals to |s|.
static bool equal(Token *tok, char *s) {
    return strlen(s) == tok->token_length &&
        !strncmp(tok->token_string, s, tok->token_length);
}

// Ensures that the current token is |s|.
static Token *skip(Token *tok, char *s) {
    if (!equal(tok, s))
        error_tok(tok, "expected '%s'.", s);
    return tok->next;
}

// Ensures that the current token is |TOKEN_NUM|.
static int take_number(Token *tok) {
    if (tok->kind != TOKEN_NUM)
        error_tok(tok, "expected a number.");
    return tok->val;
}

// Adds the token information for |kind| and |str| after |tail|.
static Token *create_new_token(Token *tail, TokenKind kind, char *token_string,
    int token_length) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->token_string = token_string;
    tok->token_length = token_length;
    tail->next = tok;
    return tok;
}

static bool prefix_matchs(char *p, char *query) {
    return strncmp(p, query, strlen(query)) == 0;
}

// Tokenize |p| and returns token's head.
static Token *tokenize() {
    char *p = current_input;
    Token head;
    head.next = NULL;
    Token *tail = &head;

    while (*p) {
        // Skips white-space characters.
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Multi-letter punctuators
        if (prefix_matchs(p, "==") || prefix_matchs(p, "!=")
            || prefix_matchs(p, "<=") || prefix_matchs(p, ">=")) {
            tail = create_new_token(tail, TOKEN_SYMBOL, p, 2);
            p += 2;
            continue;
        }

        // Single-letter punctuators
        if (strchr("+-*/()<>", *p)) {
            tail = create_new_token(tail, TOKEN_SYMBOL, p, 1);
            p++;
            continue;
        }

        // Integer literal
        if (isdigit(*p)) {
            tail = create_new_token(tail, TOKEN_NUM, p, 0);
            char *q = p;
            tail->val = strtoul(p, &p, 10);
            tail->token_length = p - q;
            continue;
        }

        error_at(p, "invalid token.");
    }

    create_new_token(tail, TOKEN_EOF, p, 0);
    return head.next;
}

void print_all_token(Token* head) {
    Token *copy_head = head;
    while (copy_head->next) {
        fprintf(stderr, "TYPE[%d], STR[%s], INT[%d]\n",
            copy_head->kind, copy_head->token_string, copy_head->val);

        copy_head = copy_head->next;
    }
    fprintf(stderr, "end. \n");
}


//
// Parser
//

typedef enum {
    NODE_ADD, // +
    NODE_SUB, // -
    NODE_MUL, // *
    NODE_DIV, // /
    NODE_EQ,  // ==
    NODE_NE,  // !=
    NODE_LT,  // <
    NODE_LE,  // <=
    NODE_GT,  // >
    NODE_GE,  // >=
    NODE_NUM, // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val; // Used if kind == NODE_NUM
};

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

//
// Code generator
//

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


int main(int argc, char **argv) {
    if (argc != 2)
        error("%s: invalid number of arguments.", argv[0]);

    // Tokenize and parse.
    current_input = argv[1];
    Token *tok = tokenize();

    Node *node = expr(&tok, tok);

    if (tok->kind != TOKEN_EOF)
        error_tok(tok, "extra token.");

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

    return 0;
}