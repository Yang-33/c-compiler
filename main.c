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

// Reports an error message and exit.
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
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
        error("It's not '%c'.", op);
    }
    current_token = current_token->next;
}

// Eusures the current token if it's a number and returns its value.
// Otherwise, it calls an error function.
int expect_number() {
    if (current_token->kind != TOKEN_NUM) {
        error("It's not a number");
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
Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // Skips white-space characters.
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Arithmetic operator
        if (*p == '+' || *p == '-') {
            cur = create_new_token(cur, TOKEN_SYMBOL, p++);
            continue;
        }

        // Integer literal
        if (isdigit(*p)) {
            cur = create_new_token(cur, TOKEN_NUM, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }
        error("Invalid token");
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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: missing command line arguments.\n");
        return 1;
    }

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    current_token = tokenize(argv[1]);

    printf("  mov rax, %d\n", expect_number());

    while (!at_eof()) {
        if (consume('+')) {
            printf("  add rax, %d\n", expect_number());
            continue;
        }

        expect('-');
        printf("  sub rax, %d\n", expect_number());
    }

    printf("  ret\n");
    return 0;
}