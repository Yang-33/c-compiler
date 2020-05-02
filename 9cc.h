#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

//
// tokenize.c
//

// Token
typedef enum {
    TOKEN_SYMBOL,
    TOKEN_IDENTIFIER,
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
bool consume(Token **rest, Token *tok, char *str);
Token *tokenize(char *p);
void print_all_token(Token *head);

//
// parse.c
//

// Local variable
typedef struct Var Var;
struct Var {
    Var *next;
    char *name; // Variable name
    Type *ty;   // Type
    int offset; // Offset from RBP
};

// AST node
typedef enum {
    NODE_ADD,             // +
    NODE_SUB,             // -
    NODE_MUL,             // *
    NODE_DIV,             // /
    NODE_EQ,              // ==
    NODE_NE,              // !=
    NODE_LT,              // <
    NODE_LE,              // <=
    NODE_GT,              // >
    NODE_GE,              // >=
    NODE_ASSIGN,          // =
    NODE_ADDRESS,         // unary &
    NODE_DEREFERENCE,     // unary *
    NODE_RETURN,          // return
    NODE_IF,              // if
    NODE_FOR,             // for or while
    NODE_BLOCK,           // { ... }
    NODE_FUNCTION_CALL,   // Function call
    NODE_EXPR_STATEMENT,  // Expression statement
    NODE_VAR,             // Variable
    NODE_NUM,             // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
    NodeKind kind;
    Node *next;    // Divided by semicolon
    Type *ty;      // Type, e.g. (pointer to)* int
    Token *tok;    // Representative token

    Node *lhs;
    Node *rhs;

    // "if or for statement"
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    // Code block
    Node *body;

    // Function call
    char *funcname;

    Var *var;      // Used if kind == NODE_VAR
    int val;       // Used if kind == NODE_NUM
};


typedef struct Function Function;
struct Function {
    Node *node;
    Var *locals;
    int stack_size;
};
Function *parse(Token *tok);

//
// type.c
//

typedef enum { TY_INT, TY_PTR } TypeKind;

struct Type {
    TypeKind kind;

    // Pointer
    Type *base;

    // Declaration
    Token *name;
};

extern Type *ty_int;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
void add_type(Node *node);


//
// codegen.c
//

void codegen(Function *prog);
