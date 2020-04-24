#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TOKEN_SYMBOL,
    TOKEN_NUM,
    TOKEN_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *str;
};

Token *current_token;
char *user_input;

// Reports an error and exit.
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Reports an error location and exit.
void compile_error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Consumes the current token if it matches |op| or an expected symbol.
// Otherwise, it returns false.
bool consume(char op) {
    if (current_token->kind != TOKEN_SYMBOL || current_token->str[0] != op) {
        return false;
    }
    current_token = current_token->next;
    return true;
}

// Eusures the current token if it matches |op| or an expected symbol.
// Otherwise, it calls an error function.
void expect(char op) {
    if (current_token->kind != TOKEN_SYMBOL || current_token->str[0] != op) {
        compile_error_at(current_token->str, "Expected '%c'.", op);
    }
    current_token = current_token->next;
}

// Eusures the current token if it's a number and returns its value.
// Otherwise, it calls an error function.
int expect_number() {
    if (current_token->kind != TOKEN_NUM) {
        compile_error_at(current_token->str, "Expected a number.");
    }
    int val = current_token->val;
    current_token = current_token->next;
    return val;
}

bool at_eof() {
    return current_token->kind == TOKEN_EOF;
}

// Adds the token information for |kind| and |str| after |tail|.
Token *create_new_token(Token* tail, TokenKind kind, char *str) {
    Token *token = calloc(1, sizeof(Token));
    token->kind = kind;
    token->str = str;
    tail->next = token;
    return token;
}

// Tokenize |p| and returns token's head.
Token *tokenize() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // Skips white-space characters.
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Symbol
        if (*p == '+' || *p == '-' || *p == '(' || *p == ')') {
            cur = create_new_token(cur, TOKEN_SYMBOL, p++);
            continue;
        }

        // Integer literal
        if (isdigit(*p)) {
            cur = create_new_token(cur, TOKEN_NUM, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }
        compile_error_at(p, "Expected a number.");
    }

    create_new_token(cur, TOKEN_EOF, p);
    return head.next;
}

void print_all_token(Token* head) {
    Token *copy_head = head;
    while (copy_head->next) {
        fprintf(stderr, "TYPE[%d], STR[%s], INT[%d]\n", copy_head->kind, copy_head->str, copy_head->val);
        copy_head = copy_head->next;
    }
    fprintf(stderr, "end. \n");
}

//
// Parser
//
typedef enum {
    NODE_ADD,
    NODE_SUB,
    //NODE_MUL,
    //NODE_DIV,
    NODE_NUM,
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val;
};

Node *create_new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

Node *create_new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = create_new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *create_new_number(int val) {
    Node *node = create_new_node(NODE_NUM);
    node->val = val;
    return node;
}

Node *expr();
Node *primary();

Node *expr() {
    Node *node = primary();
    for (;;) {
        if (consume('+')) {
            node = create_new_binary(NODE_ADD, node, primary());
        }
        else if (consume('-')) {
            node = create_new_binary(NODE_SUB, node, primary());
        }
        else {
            return node;
        }
    }
}

Node *primary() {
    if (consume('(')) {
        Node *node = expr();
        expect(')');
        return node;
    }
    return create_new_number(expect_number());
}

//
// Code generator
//
void generate_asm(Node *node) {
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
    case NODE_NUM:
        error("Internal error: invalid node. kind:= %d", node->kind);
        break;
    }

    printf("  push rax\n");
}


int main(int argc, char **argv) {
    if (argc != 2) {
        error("%s: invalid number of arguments", argv[0]);
    }

    // Tokenize and parse.
    user_input = argv[1];
    current_token = tokenize();
    Node *node = expr();

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