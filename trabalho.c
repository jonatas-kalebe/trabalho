#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================
 * Léxico: tokens e scanner
 * ========================= */
typedef enum {
    TOK_EOF = -1,
    TOK_INVALID = 0,
    TOK_PRINCIPAL,
    TOK_IDENT,
    TOK_INT,
    TOK_CAR,
    TOK_LEIA,
    TOK_ESCREVA,
    TOK_NOVALINHA,
    TOK_SE,
    TOK_ENTAO,
    TOK_SENAO,
    TOK_FIMSE,
    TOK_ENQUANTO,
    TOK_STRING,
    TOK_OU,
    TOK_E,
    TOK_IGUAL,
    TOK_DIFERENTE,
    TOK_MAIORIGUAL,
    TOK_MENORIGUAL,
    TOK_CHARCONST,
    TOK_INTCONST,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_ASSIGN,
    TOK_LT,
    TOK_GT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_BANG
} TokenKind;

typedef enum {
    TYPE_INT,
    TYPE_CAR
} Type;

typedef struct {
    TokenKind kind;
    char *lexeme;
    int line;
    int int_value;
    int char_value;
} Token;

typedef struct {
    const char *src;
    size_t len;
    size_t pos;
    int line;
} Lexer;

static void die_alloc(void) {
    fprintf(stderr, "ERRO: FALHA DE MEMORIA\n");
    exit(1);
}

static char *xstrdup(const char *s) {
    size_t n = strlen(s);
    char *r = (char *)malloc(n + 1);
    if (!r) die_alloc();
    memcpy(r, s, n + 1);
    return r;
}

static char *xsubstr(const char *s, size_t start, size_t end) {
    size_t n = end - start;
    char *r = (char *)malloc(n + 1);
    if (!r) die_alloc();
    memcpy(r, s + start, n);
    r[n] = '\0';
    return r;
}

static void lexical_error(const char *msg, int line) {
    printf("ERRO: %s %d\n", msg, line);
    exit(1);
}

static int lexer_peek(const Lexer *lx) {
    if (lx->pos >= lx->len) return '\0';
    return (unsigned char)lx->src[lx->pos];
}

static int lexer_peek2(const Lexer *lx) {
    if (lx->pos + 1 >= lx->len) return '\0';
    return (unsigned char)lx->src[lx->pos + 1];
}

static int lexer_advance(Lexer *lx) {
    if (lx->pos >= lx->len) return '\0';
    int c = (unsigned char)lx->src[lx->pos++];
    if (c == '\n') lx->line++;
    return c;
}

static int is_ident_start(int c) {
    return isalpha(c) || c == '_';
}

static int is_ident_char(int c) {
    return isalnum(c) || c == '_';
}

static Token make_simple_token(TokenKind kind, int line) {
    Token tk;
    tk.kind = kind;
    tk.lexeme = NULL;
    tk.line = line;
    tk.int_value = 0;
    tk.char_value = 0;
    return tk;
}

static TokenKind keyword_kind(const char *s) {
    if (strcmp(s, "principal") == 0) return TOK_PRINCIPAL;
    if (strcmp(s, "int") == 0) return TOK_INT;
    if (strcmp(s, "car") == 0) return TOK_CAR;
    if (strcmp(s, "leia") == 0) return TOK_LEIA;
    if (strcmp(s, "escreva") == 0) return TOK_ESCREVA;
    if (strcmp(s, "novalinha") == 0) return TOK_NOVALINHA;
    if (strcmp(s, "se") == 0) return TOK_SE;
    if (strcmp(s, "entao") == 0) return TOK_ENTAO;
    if (strcmp(s, "senao") == 0) return TOK_SENAO;
    if (strcmp(s, "fimse") == 0) return TOK_FIMSE;
    if (strcmp(s, "enquanto") == 0) return TOK_ENQUANTO;
    return TOK_IDENT;
}

static void skip_ws_comments(Lexer *lx) {
    for (;;) {
        int c = lexer_peek(lx);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v') {
            lexer_advance(lx);
            continue;
        }
        if (c == '/' && lexer_peek2(lx) == '*') {
            int start_line = lx->line;
            lexer_advance(lx);
            lexer_advance(lx);
            int closed = 0;
            while (lx->pos < lx->len) {
                int x = lexer_advance(lx);
                if (x == '*' && lexer_peek(lx) == '/') {
                    lexer_advance(lx);
                    closed = 1;
                    break;
                }
            }
            if (!closed) lexical_error("COMENTARIO NAO TERMINA", start_line);
            continue;
        }
        break;
    }
}

static Token lexer_next_token(Lexer *lx) {
    skip_ws_comments(lx);
    int line = lx->line;
    int c = lexer_peek(lx);

    if (c == '\0') return make_simple_token(TOK_EOF, line);

    if (is_ident_start(c)) {
        size_t st = lx->pos;
        lexer_advance(lx);
        while (is_ident_char(lexer_peek(lx))) lexer_advance(lx);
        char *text = xsubstr(lx->src, st, lx->pos);
        TokenKind k = keyword_kind(text);
        Token tk = make_simple_token(k, line);
        if (k == TOK_IDENT) tk.lexeme = text;
        else free(text);
        return tk;
    }

    if (isdigit(c)) {
        size_t st = lx->pos;
        while (isdigit(lexer_peek(lx))) lexer_advance(lx);
        char *text = xsubstr(lx->src, st, lx->pos);
        Token tk = make_simple_token(TOK_INTCONST, line);
        tk.int_value = atoi(text);
        tk.lexeme = text;
        return tk;
    }

    if (c == '"') {
        size_t st;
        lexer_advance(lx);
        st = lx->pos;
        while (1) {
            int x = lexer_peek(lx);
            if (x == '\0' || x == '\n') {
                lexical_error("CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA", line);
            }
            if (x == '"') break;
            lexer_advance(lx);
        }
        char *text = xsubstr(lx->src, st, lx->pos);
        lexer_advance(lx);
        Token tk = make_simple_token(TOK_STRING, line);
        tk.lexeme = text;
        return tk;
    }

    if (c == '\'') {
        lexer_advance(lx);
        int value;
        int x = lexer_peek(lx);
        if (x == '\0' || x == '\n') lexical_error("CARACTERE INVALIDO", line);
        if (x == '\\') {
            lexer_advance(lx);
            int e = lexer_peek(lx);
            if (e == '\0' || e == '\n') lexical_error("CARACTERE INVALIDO", line);
            lexer_advance(lx);
            switch (e) {
                case 'n': value = '\n'; break;
                case 't': value = '\t'; break;
                case 'r': value = '\r'; break;
                case '\\': value = '\\'; break;
                case '\'': value = '\''; break;
                case '"': value = '"'; break;
                case '0': value = '\0'; break;
                default: value = e; break;
            }
        } else {
            value = lexer_advance(lx);
        }
        if (lexer_peek(lx) != '\'') lexical_error("CARACTERE INVALIDO", line);
        lexer_advance(lx);
        Token tk = make_simple_token(TOK_CHARCONST, line);
        tk.char_value = value;
        return tk;
    }

    if (c == '|' && lexer_peek2(lx) == '|') {
        lexer_advance(lx);
        lexer_advance(lx);
        return make_simple_token(TOK_OU, line);
    }
    if (c == '=' && lexer_peek2(lx) == '=') {
        lexer_advance(lx);
        lexer_advance(lx);
        return make_simple_token(TOK_IGUAL, line);
    }
    if (c == '!' && lexer_peek2(lx) == '=') {
        lexer_advance(lx);
        lexer_advance(lx);
        return make_simple_token(TOK_DIFERENTE, line);
    }
    if (c == '>' && lexer_peek2(lx) == '=') {
        lexer_advance(lx);
        lexer_advance(lx);
        return make_simple_token(TOK_MAIORIGUAL, line);
    }
    if (c == '<' && lexer_peek2(lx) == '=') {
        lexer_advance(lx);
        lexer_advance(lx);
        return make_simple_token(TOK_MENORIGUAL, line);
    }

    lexer_advance(lx);
    switch (c) {
        case '{': return make_simple_token(TOK_LBRACE, line);
        case '}': return make_simple_token(TOK_RBRACE, line);
        case '(': return make_simple_token(TOK_LPAREN, line);
        case ')': return make_simple_token(TOK_RPAREN, line);
        case ':': return make_simple_token(TOK_COLON, line);
        case ';': return make_simple_token(TOK_SEMICOLON, line);
        case ',': return make_simple_token(TOK_COMMA, line);
        case '=': return make_simple_token(TOK_ASSIGN, line);
        case '<': return make_simple_token(TOK_LT, line);
        case '>': return make_simple_token(TOK_GT, line);
        case '+': return make_simple_token(TOK_PLUS, line);
        case '-': return make_simple_token(TOK_MINUS, line);
        case '*': return make_simple_token(TOK_STAR, line);
        case '/': return make_simple_token(TOK_SLASH, line);
        case '!': return make_simple_token(TOK_BANG, line);
        case '&': return make_simple_token(TOK_E, line);
        default: lexical_error("CARACTERE INVALIDO", line);
    }
    return make_simple_token(TOK_INVALID, line);
}

typedef enum {
    EX_VAR,
    EX_INT,
    EX_CHAR,
    EX_ASSIGN,
    EX_BINARY,
    EX_UNARY
} ExprKind;

typedef enum {
    OP_OR,
    OP_AND,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_GE,
    OP_LE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEG,
    OP_NOT
} OpKind;

typedef struct Expr Expr;

typedef struct {
    OpKind op;
    Expr *left;
    Expr *right;
} BinaryExpr;

typedef struct {
    OpKind op;
    Expr *expr;
} UnaryExpr;

struct Expr {
    ExprKind kind;
    int line;
    Type inferred_type;
    union {
        char *name;
        int int_value;
        int char_value;
        struct {
            char *name;
            Expr *value;
        } assign;
        BinaryExpr bin;
        UnaryExpr un;
    } as;
};

typedef struct Decl {
    char *name;
    Type type;
    int line;
    struct Decl *next;
} Decl;

typedef struct Block Block;

typedef enum {
    ST_EMPTY,
    ST_EXPR,
    ST_READ,
    ST_WRITE_EXPR,
    ST_WRITE_STR,
    ST_NEWLINE,
    ST_IF,
    ST_WHILE,
    ST_BLOCK
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    int line;
    struct Stmt *next;
    union {
        Expr *expr;
        char *name;
        struct {
            Expr *cond;
            struct Stmt *then_branch;
            struct Stmt *else_branch;
        } if_stmt;
        struct {
            Expr *cond;
            struct Stmt *body;
        } while_stmt;
        Block *block;
        struct {
            char *text;
            char *label;
        } write_str;
    } as;
} Stmt;

struct Block {
    Decl *decls;
    Stmt *commands;
    int line;
};

typedef struct {
    Block *block;
} Program;

/* ============================
 * Parser (descida recursiva)
 * ============================ */
typedef struct {
    Lexer lexer;
    Token current;
} Parser;

typedef struct {
    size_t pos;
    int line;
    Token current;
} ParserCheckpoint;

static void free_token(Token *tk) {
    if (tk->lexeme) free(tk->lexeme);
    tk->lexeme = NULL;
}

static void parser_advance(Parser *p) {
    free_token(&p->current);
    p->current = lexer_next_token(&p->lexer);
}

static void syntax_error(Parser *p, const char *msg) {
    printf("ERRO: %s %d\n", msg, p->current.line);
    exit(1);
}

static void parser_expect(Parser *p, TokenKind kind, const char *msg) {
    if (p->current.kind != kind) syntax_error(p, msg);
    parser_advance(p);
}

static char *parser_take_ident(Parser *p) {
    if (p->current.kind != TOK_IDENT) syntax_error(p, "ERRO SINTATICO");
    char *name = xstrdup(p->current.lexeme);
    parser_advance(p);
    return name;
}

static Expr *new_expr(ExprKind k, int line) {
    Expr *e = (Expr *)calloc(1, sizeof(Expr));
    if (!e) die_alloc();
    e->kind = k;
    e->line = line;
    e->inferred_type = TYPE_INT;
    return e;
}

static Decl *new_decl(char *name, Type type, int line) {
    Decl *d = (Decl *)calloc(1, sizeof(Decl));
    if (!d) die_alloc();
    d->name = name;
    d->type = type;
    d->line = line;
    return d;
}

static Stmt *new_stmt(StmtKind kind, int line) {
    Stmt *s = (Stmt *)calloc(1, sizeof(Stmt));
    if (!s) die_alloc();
    s->kind = kind;
    s->line = line;
    return s;
}

static Block *new_block(Decl *decls, Stmt *commands, int line) {
    Block *b = (Block *)calloc(1, sizeof(Block));
    if (!b) die_alloc();
    b->decls = decls;
    b->commands = commands;
    b->line = line;
    return b;
}

static Expr *parse_expr(Parser *p);
static Stmt *parse_command(Parser *p);
static Block *parse_block(Parser *p);

static Expr *parse_primary(Parser *p) {
    Expr *e;
    if (p->current.kind == TOK_IDENT) {
        e = new_expr(EX_VAR, p->current.line);
        e->as.name = xstrdup(p->current.lexeme);
        parser_advance(p);
        return e;
    }
    if (p->current.kind == TOK_INTCONST) {
        e = new_expr(EX_INT, p->current.line);
        e->as.int_value = p->current.int_value;
        parser_advance(p);
        return e;
    }
    if (p->current.kind == TOK_CHARCONST) {
        e = new_expr(EX_CHAR, p->current.line);
        e->as.char_value = p->current.char_value;
        parser_advance(p);
        return e;
    }
    if (p->current.kind == TOK_LPAREN) {
        parser_advance(p);
        e = parse_expr(p);
        parser_expect(p, TOK_RPAREN, "ERRO SINTATICO");
        return e;
    }
    syntax_error(p, "ERRO SINTATICO");
    return NULL;
}

static Expr *parse_unary(Parser *p) {
    if (p->current.kind == TOK_MINUS) {
        int line = p->current.line;
        parser_advance(p);
        Expr *e = new_expr(EX_UNARY, line);
        e->as.un.op = OP_NEG;
        e->as.un.expr = parse_unary(p);
        return e;
    }
    if (p->current.kind == TOK_BANG) {
        int line = p->current.line;
        parser_advance(p);
        Expr *e = new_expr(EX_UNARY, line);
        e->as.un.op = OP_NOT;
        e->as.un.expr = parse_unary(p);
        return e;
    }
    return parse_primary(p);
}

static Expr *parse_mul(Parser *p) {
    Expr *left = parse_unary(p);
    while (p->current.kind == TOK_STAR || p->current.kind == TOK_SLASH) {
        TokenKind k = p->current.kind;
        int line = p->current.line;
        parser_advance(p);
        Expr *right = parse_unary(p);
        Expr *e = new_expr(EX_BINARY, line);
        e->as.bin.op = (k == TOK_STAR) ? OP_MUL : OP_DIV;
        e->as.bin.left = left;
        e->as.bin.right = right;
        left = e;
    }
    return left;
}

static Expr *parse_add(Parser *p) {
    Expr *left = parse_mul(p);
    while (p->current.kind == TOK_PLUS || p->current.kind == TOK_MINUS) {
        TokenKind k = p->current.kind;
        int line = p->current.line;
        parser_advance(p);
        Expr *right = parse_mul(p);
        Expr *e = new_expr(EX_BINARY, line);
        e->as.bin.op = (k == TOK_PLUS) ? OP_ADD : OP_SUB;
        e->as.bin.left = left;
        e->as.bin.right = right;
        left = e;
    }
    return left;
}

static Expr *parse_cmp(Parser *p) {
    Expr *left = parse_add(p);
    while (p->current.kind == TOK_LT || p->current.kind == TOK_GT ||
           p->current.kind == TOK_MAIORIGUAL || p->current.kind == TOK_MENORIGUAL) {
        TokenKind k = p->current.kind;
        int line = p->current.line;
        parser_advance(p);
        Expr *right = parse_add(p);
        Expr *e = new_expr(EX_BINARY, line);
        if (k == TOK_LT) e->as.bin.op = OP_LT;
        else if (k == TOK_GT) e->as.bin.op = OP_GT;
        else if (k == TOK_MAIORIGUAL) e->as.bin.op = OP_GE;
        else e->as.bin.op = OP_LE;
        e->as.bin.left = left;
        e->as.bin.right = right;
        left = e;
    }
    return left;
}

static Expr *parse_eq(Parser *p) {
    Expr *left = parse_cmp(p);
    while (p->current.kind == TOK_IGUAL || p->current.kind == TOK_DIFERENTE) {
        TokenKind k = p->current.kind;
        int line = p->current.line;
        parser_advance(p);
        Expr *right = parse_cmp(p);
        Expr *e = new_expr(EX_BINARY, line);
        e->as.bin.op = (k == TOK_IGUAL) ? OP_EQ : OP_NE;
        e->as.bin.left = left;
        e->as.bin.right = right;
        left = e;
    }
    return left;
}

static Expr *parse_and(Parser *p) {
    Expr *left = parse_eq(p);
    while (p->current.kind == TOK_E) {
        int line = p->current.line;
        parser_advance(p);
        Expr *right = parse_eq(p);
        Expr *e = new_expr(EX_BINARY, line);
        e->as.bin.op = OP_AND;
        e->as.bin.left = left;
        e->as.bin.right = right;
        left = e;
    }
    return left;
}

static Expr *parse_or(Parser *p) {
    Expr *left = parse_and(p);
    while (p->current.kind == TOK_OU) {
        int line = p->current.line;
        parser_advance(p);
        Expr *right = parse_and(p);
        Expr *e = new_expr(EX_BINARY, line);
        e->as.bin.op = OP_OR;
        e->as.bin.left = left;
        e->as.bin.right = right;
        left = e;
    }
    return left;
}

static Expr *parse_expr(Parser *p) {
    Expr *left = parse_or(p);
    if (p->current.kind == TOK_ASSIGN) {
        if (left->kind != EX_VAR) syntax_error(p, "ERRO SINTATICO");
        int line = p->current.line;
        char *name = xstrdup(left->as.name);
        parser_advance(p);
        Expr *rhs = parse_expr(p);
        Expr *e = new_expr(EX_ASSIGN, line);
        e->as.assign.name = name;
        e->as.assign.value = rhs;
        return e;
    }
    return left;
}

static Stmt *parse_command_list(Parser *p) {
    Stmt *head = NULL;
    Stmt *tail = NULL;
    if (p->current.kind == TOK_RBRACE) syntax_error(p, "ERRO SINTATICO");
    while (p->current.kind != TOK_RBRACE && p->current.kind != TOK_EOF) {
        Stmt *s = parse_command(p);
        if (!head) head = tail = s;
        else {
            tail->next = s;
            tail = s;
        }
    }
    return head;
}

/* Salva apenas o estado necessário para voltar ao mesmo ponto do input. */
static void parser_checkpoint_save(Parser *p, ParserCheckpoint *cp) {
    cp->pos = p->lexer.pos;
    cp->line = p->lexer.line;
    cp->current = p->current;
    if (p->current.lexeme) cp->current.lexeme = xstrdup(p->current.lexeme);
}

static void parser_checkpoint_restore(Parser *p, ParserCheckpoint *cp) {
    free_token(&p->current);
    p->lexer.pos = cp->pos;
    p->lexer.line = cp->line;
    p->current = cp->current;
}

static int try_parse_var_section(Parser *p, Decl **out_decls) {
    ParserCheckpoint cp;
    parser_checkpoint_save(p, &cp);

    if (p->current.kind != TOK_LBRACE) {
        free_token(&cp.current);
        return 0;
    }
    parser_advance(p);
    if (p->current.kind != TOK_IDENT) {
        parser_checkpoint_restore(p, &cp);
        return 0;
    }

    Decl *head = NULL;
    Decl *tail = NULL;

    while (1) {
        /* IDENTIFICADOR (',' IDENTIFICADOR)* ':' Tipo ';' */
        char **names = NULL;
        int names_cap = 0;
        int names_sz = 0;
        int line = p->current.line;

        while (1) {
            if (p->current.kind != TOK_IDENT) {
                parser_checkpoint_restore(p, &cp);
                return 0;
            }
            if (names_sz == names_cap) {
                names_cap = names_cap ? names_cap * 2 : 4;
                char **tmp = (char **)realloc(names, (size_t)names_cap * sizeof(char *));
                if (!tmp) die_alloc();
                names = tmp;
            }
            names[names_sz++] = xstrdup(p->current.lexeme);
            parser_advance(p);
            if (p->current.kind != TOK_COMMA) break;
            parser_advance(p);
        }

        if (p->current.kind != TOK_COLON) {
            for (int i = 0; i < names_sz; i++) free(names[i]);
            free(names);
            parser_checkpoint_restore(p, &cp);
            return 0;
        }
        parser_advance(p);

        Type type;
        if (p->current.kind == TOK_INT) {
            type = TYPE_INT;
            parser_advance(p);
        } else if (p->current.kind == TOK_CAR) {
            type = TYPE_CAR;
            parser_advance(p);
        } else {
            for (int i = 0; i < names_sz; i++) free(names[i]);
            free(names);
            parser_checkpoint_restore(p, &cp);
            return 0;
        }

        if (p->current.kind != TOK_SEMICOLON) {
            for (int i = 0; i < names_sz; i++) free(names[i]);
            free(names);
            parser_checkpoint_restore(p, &cp);
            return 0;
        }
        parser_advance(p);

        for (int i = 0; i < names_sz; i++) {
            Decl *d = new_decl(names[i], type, line);
            if (!head) head = tail = d;
            else {
                tail->next = d;
                tail = d;
            }
        }
        free(names);

        if (p->current.kind != TOK_IDENT) break;
    }

    if (p->current.kind != TOK_RBRACE) {
        parser_checkpoint_restore(p, &cp);
        return 0;
    }
    parser_advance(p);

    if (p->current.kind != TOK_LBRACE) {
        parser_checkpoint_restore(p, &cp);
        return 0;
    }

    free_token(&cp.current);
    *out_decls = head;
    return 1;
}

static Block *parse_block(Parser *p) {
    if (p->current.kind != TOK_LBRACE) syntax_error(p, "ERRO SINTATICO");
    int line = p->current.line;

    Decl *decls = NULL;
    /* Bloco pode ser:
     *   { ListaComando }
     * ou
     *   { ListaDeclVar } { ListaComando }
     */
    if (try_parse_var_section(p, &decls)) {
        parser_expect(p, TOK_LBRACE, "ERRO SINTATICO");
        Stmt *cmds = parse_command_list(p);
        parser_expect(p, TOK_RBRACE, "ERRO SINTATICO");
        return new_block(decls, cmds, line);
    }

    parser_expect(p, TOK_LBRACE, "ERRO SINTATICO");
    Stmt *cmds = parse_command_list(p);
    parser_expect(p, TOK_RBRACE, "ERRO SINTATICO");
    return new_block(NULL, cmds, line);
}

static Stmt *parse_command(Parser *p) {
    if (p->current.kind == TOK_SEMICOLON) {
        int line = p->current.line;
        parser_advance(p);
        return new_stmt(ST_EMPTY, line);
    }
    if (p->current.kind == TOK_LEIA) {
        int line = p->current.line;
        parser_advance(p);
        char *name = parser_take_ident(p);
        parser_expect(p, TOK_SEMICOLON, "ERRO SINTATICO");
        Stmt *s = new_stmt(ST_READ, line);
        s->as.name = name;
        return s;
    }
    if (p->current.kind == TOK_ESCREVA) {
        int line = p->current.line;
        parser_advance(p);
        if (p->current.kind == TOK_STRING) {
            Stmt *s = new_stmt(ST_WRITE_STR, line);
            s->as.write_str.text = xstrdup(p->current.lexeme);
            parser_advance(p);
            parser_expect(p, TOK_SEMICOLON, "ERRO SINTATICO");
            return s;
        }
        Stmt *s = new_stmt(ST_WRITE_EXPR, line);
        s->as.expr = parse_expr(p);
        parser_expect(p, TOK_SEMICOLON, "ERRO SINTATICO");
        return s;
    }
    if (p->current.kind == TOK_NOVALINHA) {
        int line = p->current.line;
        parser_advance(p);
        parser_expect(p, TOK_SEMICOLON, "ERRO SINTATICO");
        return new_stmt(ST_NEWLINE, line);
    }
    if (p->current.kind == TOK_SE) {
        int line = p->current.line;
        parser_advance(p);
        parser_expect(p, TOK_LPAREN, "ERRO SINTATICO");
        Expr *cond = parse_expr(p);
        parser_expect(p, TOK_RPAREN, "ERRO SINTATICO");
        parser_expect(p, TOK_ENTAO, "ERRO SINTATICO");
        Stmt *then_branch = parse_command(p);
        Stmt *else_branch = NULL;
        if (p->current.kind == TOK_SENAO) {
            parser_advance(p);
            else_branch = parse_command(p);
        }
        parser_expect(p, TOK_FIMSE, "ERRO SINTATICO");
        Stmt *s = new_stmt(ST_IF, line);
        s->as.if_stmt.cond = cond;
        s->as.if_stmt.then_branch = then_branch;
        s->as.if_stmt.else_branch = else_branch;
        return s;
    }
    if (p->current.kind == TOK_ENQUANTO) {
        int line = p->current.line;
        parser_advance(p);
        parser_expect(p, TOK_LPAREN, "ERRO SINTATICO");
        Expr *cond = parse_expr(p);
        parser_expect(p, TOK_RPAREN, "ERRO SINTATICO");
        Stmt *body = parse_command(p);
        Stmt *s = new_stmt(ST_WHILE, line);
        s->as.while_stmt.cond = cond;
        s->as.while_stmt.body = body;
        return s;
    }
    if (p->current.kind == TOK_LBRACE) {
        Stmt *s = new_stmt(ST_BLOCK, p->current.line);
        s->as.block = parse_block(p);
        return s;
    }

    Stmt *s = new_stmt(ST_EXPR, p->current.line);
    s->as.expr = parse_expr(p);
    parser_expect(p, TOK_SEMICOLON, "ERRO SINTATICO");
    return s;
}

static Program parse_program(Parser *p) {
    Program prog;
    if (p->current.kind != TOK_PRINCIPAL) syntax_error(p, "ERRO SINTATICO");
    parser_advance(p);
    prog.block = parse_block(p);
    if (p->current.kind != TOK_EOF) syntax_error(p, "ERRO SINTATICO");
    return prog;
}

typedef struct Sym {
    char *name;
    Type type;
    int offset;
    int size;
    struct Sym *next;
} Sym;

typedef struct Scope {
    Sym *symbols;
    struct Scope *prev;
} Scope;

typedef struct {
    Scope *top;
} SemanticCtx;

/* ============================
 * Semântico (escopo e tipos)
 * ============================ */
static int type_size(Type t) {
    return (t == TYPE_INT) ? 4 : 1;
}

static void semantic_error(int line, const char *msg) {
    printf("ERRO: %s %d\n", msg, line);
    exit(1);
}

static Scope *push_scope(Scope *top) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    if (!s) die_alloc();
    s->prev = top;
    return s;
}

static Scope *pop_scope(Scope *top) {
    Scope *p = top->prev;
    while (top->symbols) {
        Sym *n = top->symbols->next;
        free(top->symbols);
        top->symbols = n;
    }
    free(top);
    return p;
}

static Sym *lookup_scope(Scope *s, const char *name) {
    for (Sym *it = s ? s->symbols : NULL; it; it = it->next) {
        if (strcmp(it->name, name) == 0) return it;
    }
    return NULL;
}

static Sym *lookup_all(Scope *top, const char *name) {
    for (Scope *s = top; s; s = s->prev) {
        Sym *f = lookup_scope(s, name);
        if (f) return f;
    }
    return NULL;
}

static void add_symbol(Scope *scope, char *name, Type type, int line) {
    /* add_symbol recebe a posse de 'name' e libera ao destruir o escopo. */
    if (lookup_scope(scope, name)) semantic_error(line, "IDENTIFICADOR JA DECLARADO");
    Sym *s = (Sym *)calloc(1, sizeof(Sym));
    if (!s) die_alloc();
    s->name = name;
    s->type = type;
    s->size = type_size(type);
    s->next = scope->symbols;
    scope->symbols = s;
}

static Type analyze_expr(SemanticCtx *ctx, Expr *e);
static void analyze_stmt(SemanticCtx *ctx, Stmt *s);

static void analyze_block(SemanticCtx *ctx, Block *b) {
    ctx->top = push_scope(ctx->top);

    for (Decl *d = b->decls; d; d = d->next) {
        add_symbol(ctx->top, d->name, d->type, d->line);
    }

    for (Stmt *s = b->commands; s; s = s->next) {
        analyze_stmt(ctx, s);
    }

    ctx->top = pop_scope(ctx->top);
}

static Type analyze_expr(SemanticCtx *ctx, Expr *e) {
    switch (e->kind) {
        case EX_VAR: {
            Sym *sym = lookup_all(ctx->top, e->as.name);
            if (!sym) semantic_error(e->line, "IDENTIFICADOR NAO DECLARADO");
            e->inferred_type = sym->type;
            return e->inferred_type;
        }
        case EX_INT:
            e->inferred_type = TYPE_INT;
            return TYPE_INT;
        case EX_CHAR:
            e->inferred_type = TYPE_CAR;
            return TYPE_CAR;
        case EX_ASSIGN: {
            Sym *sym = lookup_all(ctx->top, e->as.assign.name);
            if (!sym) semantic_error(e->line, "IDENTIFICADOR NAO DECLARADO");
            Type rhs = analyze_expr(ctx, e->as.assign.value);
            if (rhs != sym->type) semantic_error(e->line, "ATRIBUICAO COM TIPOS DIFERENTES");
            e->inferred_type = sym->type;
            return e->inferred_type;
        }
        case EX_UNARY: {
            Type t = analyze_expr(ctx, e->as.un.expr);
            if (t != TYPE_INT) semantic_error(e->line, "OPERACAO INVALIDA ENTRE TIPOS");
            e->inferred_type = TYPE_INT;
            return TYPE_INT;
        }
        case EX_BINARY: {
            Type l = analyze_expr(ctx, e->as.bin.left);
            Type r = analyze_expr(ctx, e->as.bin.right);
            switch (e->as.bin.op) {
                case OP_ADD:
                case OP_SUB:
                case OP_MUL:
                case OP_DIV:
                    if (l != TYPE_INT || r != TYPE_INT) {
                        semantic_error(e->line, "OPERACAO INVALIDA ENTRE TIPOS");
                    }
                    e->inferred_type = TYPE_INT;
                    return TYPE_INT;
                case OP_OR:
                case OP_AND:
                    if (l != TYPE_INT || r != TYPE_INT) {
                        semantic_error(e->line, "OPERACAO INVALIDA ENTRE TIPOS");
                    }
                    e->inferred_type = TYPE_INT;
                    return TYPE_INT;
                case OP_EQ:
                case OP_NE:
                case OP_LT:
                case OP_GT:
                case OP_GE:
                case OP_LE:
                    if (l != r) semantic_error(e->line, "OPERACAO INVALIDA ENTRE TIPOS");
                    e->inferred_type = TYPE_INT;
                    return TYPE_INT;
                default:
                    break;
            }
        }
    }
    semantic_error(e->line, "ERRO SEMANTICO");
    return TYPE_INT;
}

static void analyze_stmt(SemanticCtx *ctx, Stmt *s) {
    switch (s->kind) {
        case ST_EMPTY:
            break;
        case ST_EXPR:
            (void)analyze_expr(ctx, s->as.expr);
            break;
        case ST_READ: {
            Sym *sym = lookup_all(ctx->top, s->as.name);
            if (!sym) semantic_error(s->line, "IDENTIFICADOR NAO DECLARADO");
            break;
        }
        case ST_WRITE_EXPR:
            (void)analyze_expr(ctx, s->as.expr);
            break;
        case ST_WRITE_STR:
        case ST_NEWLINE:
            break;
        case ST_IF: {
            Type t = analyze_expr(ctx, s->as.if_stmt.cond);
            if (t != TYPE_INT) semantic_error(s->line, "CONDICAO DEVE SER INT");
            analyze_stmt(ctx, s->as.if_stmt.then_branch);
            if (s->as.if_stmt.else_branch) analyze_stmt(ctx, s->as.if_stmt.else_branch);
            break;
        }
        case ST_WHILE: {
            Type t = analyze_expr(ctx, s->as.while_stmt.cond);
            if (t != TYPE_INT) semantic_error(s->line, "CONDICAO DEVE SER INT");
            analyze_stmt(ctx, s->as.while_stmt.body);
            break;
        }
        case ST_BLOCK:
            analyze_block(ctx, s->as.block);
            break;
    }
}

typedef struct CGString {
    char *text;
    char *label;
    struct CGString *next;
} CGString;

typedef struct CGScope {
    Sym *symbols;
    int alloc_size;
    struct CGScope *prev;
} CGScope;

typedef struct {
    FILE *out;
    CGString *strings;
    int string_count;
    int label_count;
    CGScope *top;
    int depth;
} CodegenCtx;

/* ============================
 * Geração de código MIPS
 * ============================ */
static Sym *cg_lookup(CodegenCtx *cg, const char *name) {
    for (CGScope *s = cg->top; s; s = s->prev) {
        for (Sym *it = s->symbols; it; it = it->next) {
            if (strcmp(it->name, name) == 0) return it;
        }
    }
    return NULL;
}

static char *new_label(CodegenCtx *cg, const char *prefix) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s_%d", prefix, cg->label_count++);
    return xstrdup(buf);
}

static char *mips_escape_string(const char *s) {
    size_t n = 0;
    for (const char *p = s; *p; p++) {
        if (*p == '\\' || *p == '"') n += 2;
        else if (*p == '\n' || *p == '\t' || *p == '\r') n += 2;
        else n += 1;
    }
    char *out = (char *)malloc(n + 1);
    if (!out) die_alloc();
    char *w = out;
    for (const char *p = s; *p; p++) {
        if (*p == '\\') {
            *w++ = '\\';
            *w++ = '\\';
        } else if (*p == '"') {
            *w++ = '\\';
            *w++ = '"';
        } else if (*p == '\n') {
            *w++ = '\\';
            *w++ = 'n';
        } else if (*p == '\t') {
            *w++ = '\\';
            *w++ = 't';
        } else if (*p == '\r') {
            *w++ = '\\';
            *w++ = 'r';
        } else {
            *w++ = *p;
        }
    }
    *w = '\0';
    return out;
}

static char *intern_string(CodegenCtx *cg, const char *text) {
    for (CGString *it = cg->strings; it; it = it->next) {
        if (strcmp(it->text, text) == 0) return it->label;
    }
    CGString *s = (CGString *)calloc(1, sizeof(CGString));
    if (!s) die_alloc();
    s->text = xstrdup(text);
    s->label = new_label(cg, "str");
    s->next = cg->strings;
    cg->strings = s;
    cg->string_count++;
    return s->label;
}

static void collect_strings_stmt(CodegenCtx *cg, Stmt *s);

static void collect_strings_block(CodegenCtx *cg, Block *b) {
    for (Stmt *s = b->commands; s; s = s->next) collect_strings_stmt(cg, s);
}

static void collect_strings_stmt(CodegenCtx *cg, Stmt *s) {
    if (s->kind == ST_WRITE_STR) {
        s->as.write_str.label = intern_string(cg, s->as.write_str.text);
    } else if (s->kind == ST_IF) {
        collect_strings_stmt(cg, s->as.if_stmt.then_branch);
        if (s->as.if_stmt.else_branch) collect_strings_stmt(cg, s->as.if_stmt.else_branch);
    } else if (s->kind == ST_WHILE) {
        collect_strings_stmt(cg, s->as.while_stmt.body);
    } else if (s->kind == ST_BLOCK) {
        collect_strings_block(cg, s->as.block);
    }
}

static void emit_expr(CodegenCtx *cg, Expr *e);
static void emit_stmt(CodegenCtx *cg, Stmt *s);

static void cg_push_scope(CodegenCtx *cg, Block *b) {
    CGScope *sc = (CGScope *)calloc(1, sizeof(CGScope));
    if (!sc) die_alloc();
    sc->prev = cg->top;
    cg->top = sc;

    int local = 0;
    for (Decl *d = b->decls; d; d = d->next) {
        local += type_size(d->type);
        Sym *sym = (Sym *)calloc(1, sizeof(Sym));
        if (!sym) die_alloc();
        sym->name = d->name;
        sym->type = d->type;
        sym->size = type_size(d->type);
        sym->offset = -(cg->depth + local);
        sym->next = sc->symbols;
        sc->symbols = sym;
    }

    sc->alloc_size = local;
    if (local > 0) {
        fprintf(cg->out, "  addiu $sp, $sp, -%d\n", local);
        cg->depth += local;
    }
}

static void cg_pop_scope(CodegenCtx *cg) {
    CGScope *sc = cg->top;
    if (!sc) return;
    if (sc->alloc_size > 0) {
        fprintf(cg->out, "  addiu $sp, $sp, %d\n", sc->alloc_size);
        cg->depth -= sc->alloc_size;
    }
    cg->top = sc->prev;
    while (sc->symbols) {
        Sym *n = sc->symbols->next;
        free(sc->symbols);
        sc->symbols = n;
    }
    free(sc);
}

static void emit_block(CodegenCtx *cg, Block *b) {
    cg_push_scope(cg, b);
    for (Stmt *s = b->commands; s; s = s->next) emit_stmt(cg, s);
    cg_pop_scope(cg);
}

static void emit_expr(CodegenCtx *cg, Expr *e) {
    switch (e->kind) {
        case EX_INT:
            fprintf(cg->out, "  li $v0, %d\n", e->as.int_value);
            return;
        case EX_CHAR:
            fprintf(cg->out, "  li $v0, %d\n", e->as.char_value);
            return;
        case EX_VAR: {
            Sym *sym = cg_lookup(cg, e->as.name);
            if (sym->type == TYPE_INT) fprintf(cg->out, "  lw $v0, %d($fp)\n", sym->offset);
            /* 'car' usa lbu para evitar extensão de sinal indevida. */
            else fprintf(cg->out, "  lbu $v0, %d($fp)\n", sym->offset);
            return;
        }
        case EX_ASSIGN: {
            Sym *sym = cg_lookup(cg, e->as.assign.name);
            emit_expr(cg, e->as.assign.value);
            if (sym->type == TYPE_INT) fprintf(cg->out, "  sw $v0, %d($fp)\n", sym->offset);
            else fprintf(cg->out, "  sb $v0, %d($fp)\n", sym->offset);
            return;
        }
        case EX_UNARY:
            emit_expr(cg, e->as.un.expr);
            if (e->as.un.op == OP_NEG) fprintf(cg->out, "  subu $v0, $zero, $v0\n");
            else fprintf(cg->out, "  seq $v0, $v0, $zero\n");
            return;
        case EX_BINARY:
            emit_expr(cg, e->as.bin.left);
            fprintf(cg->out, "  addiu $sp, $sp, -4\n");
            fprintf(cg->out, "  sw $v0, 0($sp)\n");
            emit_expr(cg, e->as.bin.right);
            fprintf(cg->out, "  move $t1, $v0\n");
            fprintf(cg->out, "  lw $t0, 0($sp)\n");
            fprintf(cg->out, "  addiu $sp, $sp, 4\n");
            switch (e->as.bin.op) {
                case OP_ADD: fprintf(cg->out, "  addu $v0, $t0, $t1\n"); break;
                case OP_SUB: fprintf(cg->out, "  subu $v0, $t0, $t1\n"); break;
                case OP_MUL: fprintf(cg->out, "  mul $v0, $t0, $t1\n"); break;
                case OP_DIV:
                    fprintf(cg->out, "  div $t0, $t1\n");
                    fprintf(cg->out, "  mflo $v0\n");
                    break;
                case OP_LT: fprintf(cg->out, "  slt $v0, $t0, $t1\n"); break;
                case OP_GT: fprintf(cg->out, "  slt $v0, $t1, $t0\n"); break;
                case OP_GE: fprintf(cg->out, "  slt $v0, $t0, $t1\n  xori $v0, $v0, 1\n"); break;
                case OP_LE: fprintf(cg->out, "  slt $v0, $t1, $t0\n  xori $v0, $v0, 1\n"); break;
                case OP_EQ: fprintf(cg->out, "  seq $v0, $t0, $t1\n"); break;
                case OP_NE: fprintf(cg->out, "  sne $v0, $t0, $t1\n"); break;
                case OP_AND:
                    fprintf(cg->out, "  sne $t0, $t0, $zero\n");
                    fprintf(cg->out, "  sne $t1, $t1, $zero\n");
                    fprintf(cg->out, "  and $v0, $t0, $t1\n");
                    break;
                case OP_OR:
                    fprintf(cg->out, "  sne $t0, $t0, $zero\n");
                    fprintf(cg->out, "  sne $t1, $t1, $zero\n");
                    fprintf(cg->out, "  or $v0, $t0, $t1\n");
                    break;
                default:
                    break;
            }
            return;
    }
}

static void emit_stmt(CodegenCtx *cg, Stmt *s) {
    switch (s->kind) {
        case ST_EMPTY:
            return;
        case ST_EXPR:
            emit_expr(cg, s->as.expr);
            return;
        case ST_READ: {
            Sym *sym = cg_lookup(cg, s->as.name);
            if (sym->type == TYPE_INT) {
                fprintf(cg->out, "  li $v0, 5\n  syscall\n");
                fprintf(cg->out, "  sw $v0, %d($fp)\n", sym->offset);
            } else {
                fprintf(cg->out, "  li $v0, 12\n  syscall\n");
                fprintf(cg->out, "  sb $v0, %d($fp)\n", sym->offset);
            }
            return;
        }
        case ST_WRITE_EXPR:
            emit_expr(cg, s->as.expr);
            if (s->as.expr->inferred_type == TYPE_INT) {
                fprintf(cg->out, "  move $a0, $v0\n  li $v0, 1\n  syscall\n");
            } else {
                fprintf(cg->out, "  move $a0, $v0\n  li $v0, 11\n  syscall\n");
            }
            return;
        case ST_WRITE_STR:
            fprintf(cg->out, "  la $a0, %s\n  li $v0, 4\n  syscall\n", s->as.write_str.label);
            return;
        case ST_NEWLINE:
            fprintf(cg->out, "  li $a0, 10\n  li $v0, 11\n  syscall\n");
            return;
        case ST_IF: {
            char *l_else = new_label(cg, "else");
            char *l_end = new_label(cg, "endif");
            emit_expr(cg, s->as.if_stmt.cond);
            if (s->as.if_stmt.else_branch) {
                fprintf(cg->out, "  beq $v0, $zero, %s\n", l_else);
                emit_stmt(cg, s->as.if_stmt.then_branch);
                fprintf(cg->out, "  j %s\n", l_end);
                fprintf(cg->out, "%s:\n", l_else);
                emit_stmt(cg, s->as.if_stmt.else_branch);
                fprintf(cg->out, "%s:\n", l_end);
            } else {
                fprintf(cg->out, "  beq $v0, $zero, %s\n", l_end);
                emit_stmt(cg, s->as.if_stmt.then_branch);
                fprintf(cg->out, "%s:\n", l_end);
            }
            free(l_else);
            free(l_end);
            return;
        }
        case ST_WHILE: {
            char *l_start = new_label(cg, "while_start");
            char *l_end = new_label(cg, "while_end");
            fprintf(cg->out, "%s:\n", l_start);
            emit_expr(cg, s->as.while_stmt.cond);
            fprintf(cg->out, "  beq $v0, $zero, %s\n", l_end);
            emit_stmt(cg, s->as.while_stmt.body);
            fprintf(cg->out, "  j %s\n", l_start);
            fprintf(cg->out, "%s:\n", l_end);
            free(l_start);
            free(l_end);
            return;
        }
        case ST_BLOCK:
            emit_block(cg, s->as.block);
            return;
    }
}

static void generate_code(Program *prog, FILE *out) {
    CodegenCtx cg;
    memset(&cg, 0, sizeof(cg));
    cg.out = out;

    collect_strings_block(&cg, prog->block);

    fprintf(out, ".data\n");
    for (CGString *it = cg.strings; it; it = it->next) {
        char *esc = mips_escape_string(it->text);
        fprintf(out, "%s: .asciiz \"%s\"\n", it->label, esc);
        free(esc);
    }

    fprintf(out, ".text\n");
    fprintf(out, ".globl main\n");
    fprintf(out, "main:\n");
    fprintf(out, "  move $fp, $sp\n");

    emit_block(&cg, prog->block);

    fprintf(out, "  li $v0, 10\n");
    fprintf(out, "  syscall\n");
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("ERRO: NAO FOI POSSIVEL ABRIR ARQUIVO\n");
        exit(1);
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        printf("ERRO: NAO FOI POSSIVEL LER ARQUIVO\n");
        exit(1);
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        printf("ERRO: NAO FOI POSSIVEL LER ARQUIVO\n");
        exit(1);
    }
    rewind(f);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) die_alloc();
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fclose(f);
        free(buf);
        printf("ERRO: NAO FOI POSSIVEL LER ARQUIVO\n");
        exit(1);
    }
    fclose(f);
    buf[sz] = '\0';
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo_entrada.g> [arquivo_saida.asm]\n", argv[0]);
        return 1;
    }

    char *source = read_file(argv[1]);

    Parser parser;
    memset(&parser, 0, sizeof(parser));
    parser.lexer.src = source;
    parser.lexer.len = strlen(source);
    parser.lexer.pos = 0;
    parser.lexer.line = 1;
    parser.current = lexer_next_token(&parser.lexer);

    Program prog = parse_program(&parser);

    SemanticCtx sem;
    sem.top = NULL;
    analyze_block(&sem, prog.block);

    FILE *out = stdout;
    if (argc >= 3) {
        out = fopen(argv[2], "w");
        if (!out) {
            printf("ERRO: NAO FOI POSSIVEL ABRIR SAIDA\n");
            return 1;
        }
    }

    generate_code(&prog, out);

    if (argc >= 3 && out) fclose(out);
    free_token(&parser.current);
    free(source);
    return 0;
}
