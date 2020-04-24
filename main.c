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
    int val; // If kind is TOKEN_NUM, it's assigned
    char *token_string;
    size_t token_length;
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
bool consume(char *op) {
    if (current_token->kind != TOKEN_SYMBOL
        || strlen(op) != current_token->token_length
        || memcmp(current_token->token_string, op, current_token->token_length))
    {
        return false;
    }
    current_token = current_token->next;
    return true;
}

// Eusures the current token if it matches |op| or an expected symbol.
// Otherwise, it calls an error function.
void expect(char *op) {
    if (current_token->kind != TOKEN_SYMBOL
        || strlen(op) != current_token->token_length
        || memcmp(current_token->token_string, op, current_token->token_length))
    {
        compile_error_at(current_token->token_string, "expected \"%s\".", op);
    }
    current_token = current_token->next;
}

// Eusures the current token if it's a number and returns its value.
// Otherwise, it calls an error function.
int expect_number() {
    if (current_token->kind != TOKEN_NUM) {
        compile_error_at(current_token->token_string, "expected a number.");
    }
    int val = current_token->val;
    current_token = current_token->next;
    return val;
}

bool at_eof() {
    return current_token->kind == TOKEN_EOF;
}

// Adds the token information for |kind| and |str| after |tail|.
Token *create_new_token(Token* tail, TokenKind kind, char *token_string,
    int token_length) {

    Token *token = calloc(1, sizeof(Token));
    token->kind = kind;
    token->token_string = token_string;
    token->token_length = token_length;
    tail->next = token;
    return token;
}

bool prefix_matchs(char *p, char *query) {
    return memcmp(p, query, strlen(query)) == 0;
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

        // Multi-letter symbol
        if (prefix_matchs(p, "==") || prefix_matchs(p, "!=")
            || prefix_matchs(p, "<=") || prefix_matchs(p, ">=")) {
            cur = create_new_token(cur, TOKEN_SYMBOL, p, 2);
            p += 2;
            continue;
        }

        // Single-letter symbol
        if (strchr("+-*/()<>", *p)) {
            cur = create_new_token(cur, TOKEN_SYMBOL, p, 1);
            p++;
            continue;
        }

        // Integer literal
        if (isdigit(*p)) {
            cur = create_new_token(cur, TOKEN_NUM, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->token_length = p - q;
            continue;
        }
        compile_error_at(p, "Invalid token");
    }

    create_new_token(cur, TOKEN_EOF, p, 0);
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
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_EQ,
    NODE_NE,
    NODE_LT,
    NODE_LE,
    NODE_GT,
    NODE_GE,
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
Node *equality();
Node* relational();
Node* add();
Node *mul();
Node *unary();
Node *primary();

// expr = equality
Node *expr() {
    return equality();
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
    Node *node = relational();
    for (;;) {
        if (consume("==")) {
            node = create_new_binary(NODE_EQ, node, relational());
        }
        else if (consume("!=")) {
            node = create_new_binary(NODE_NE, node, relational());
        }
        else {
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
    Node *node = add();
    for (;;) {
        if (consume("<")) {
            node = create_new_binary(NODE_LT, node, add());
        }
        else if (consume("<=")) {
            node = create_new_binary(NODE_LE, node, add());
        }
        else if (consume(">")) {
            node = create_new_binary(NODE_GT, node, add());
        }
        else if (consume(">=")) {
            node = create_new_binary(NODE_GE, node, add());
        }
        else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
    Node *node = mul();

    for (;;) {
        if (consume("+")) {
            node = create_new_binary(NODE_ADD, node, mul());
        }
        else if (consume("-")) {
            node = create_new_binary(NODE_SUB, node, mul());
        }
        else {
            return node;
        }
    }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
    Node *node = unary();
    for (;;) {
        if (consume("*")) {
            node = create_new_binary(NODE_MUL, node, unary());
        }
        else if (consume("/")) {
            node = create_new_binary(NODE_DIV, node, unary());
        }
        else {
            return node;
        }
    }
}

// unary = ("+" | "-")? unary | primary
Node *unary() {
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return create_new_binary(NODE_SUB, create_new_number(0), unary());
    }
    return primary();
}

// primary = "(" expr ")" | num
Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
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