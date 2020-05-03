#include "y3c.h"

// Input string
static char *current_input;

// Reports an error and exit.
void error(char *fmt, ...) {
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

void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    compile_error_at(tok->token_string, fmt, ap);
}

// Check that the current token equals to |s|.
bool equal(Token *tok, char *s) {
    return strlen(s) == tok->token_length &&
        !strncmp(tok->token_string, s, tok->token_length);
}

// Ensures that the current token is |s|.
Token *skip(Token *tok, char *s) {
    if (!equal(tok, s))
        error_tok(tok, "expected '%s'.", s);
    return tok->next;
}

bool consume(Token **rest, Token *tok, char *str) {
    if (equal(tok, str)) {
        *rest = tok->next;
        return true;
    }
    *rest = tok;
    return false;
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

static bool is_alpha_or_underscore(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum_or_underscore(char c) {
    return is_alpha_or_underscore(c) || ('0' <= c && c <= '9');
}

static int is_keyword(char *p) {
    static char *kw[] = { "return", "if", "else", "for", "while", "int" };
    for (int i = 0; i < (int)(sizeof(kw) / sizeof(*kw)); ++i) {
        int n = strlen(kw[i]);
        if (prefix_matchs(p, kw[i]) && !is_alnum_or_underscore(p[n])) {
            return n;
        }
    }
    return false;
}

// Tokenize |p| and returns token's head.
Token *tokenize(char *p) {
    current_input = p;
    Token head;
    head.next = NULL;
    Token *tail = &head;

    while (*p) {
        // Skips white-space characters.
        if (isspace(*p)) {
            p++;
            continue;
        }

        // Keyword
        if (is_keyword(p)) {
            int keyword_length = is_keyword(p);
            tail = create_new_token(tail, TOKEN_SYMBOL, p, keyword_length);
            p += keyword_length;
            continue;
        }

        // Identifier
        if (is_alpha_or_underscore(*p)) {
            char *q = p;
            while (is_alnum_or_underscore(*p)) {
                p++;
            }
            tail = create_new_token(tail, TOKEN_IDENTIFIER, q, p - q);
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
        if (strchr("+-*/&(){}<>=,;", *p)) {
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
