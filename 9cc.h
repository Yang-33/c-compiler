#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
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

void error(char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *s);
Token *skip(Token *tok, char *s);
Token *tokenize(char *p);
void print_all_token(Token *head);

//
// parse.c
//

typedef enum {
    NODE_ADD,        // +
    NODE_SUB,        // -
    NODE_MUL,        // *
    NODE_DIV,        // /
    NODE_EQ,         // ==
    NODE_NE,         // !=
    NODE_LT,         // <
    NODE_LE,         // <=
    NODE_GT,         // >
    NODE_GE,         // >=
    NODE_SEMICOLON,  // ;
    NODE_NUM,        // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *next; // Divided by semicolon
    Node *lhs;
    Node *rhs;
    int val; // Used if kind == NODE_NUM
};

Node *parse(Token *tok);

//
// codegen.c
//

void codegen(Node *node);
