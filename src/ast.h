#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <string.h>

typedef enum {
    TYPE_INT,
    TYPE_CAR
} Type;

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

// Function declarations for AST node creation
Expr *new_expr_var(char *name, int line);
Expr *new_expr_int(int value, int line);
Expr *new_expr_char(int value, int line);
Expr *new_expr_assign(char *name, Expr *value, int line);
Expr *new_expr_binary(OpKind op, Expr *left, Expr *right, int line);
Expr *new_expr_unary(OpKind op, Expr *expr, int line);

Decl *new_decl(char *name, Type type, int line);

Stmt *new_stmt_empty(int line);
Stmt *new_stmt_expr(Expr *expr, int line);
Stmt *new_stmt_read(char *name, int line);
Stmt *new_stmt_write_expr(Expr *expr, int line);
Stmt *new_stmt_write_str(char *text, int line);
Stmt *new_stmt_newline(int line);
Stmt *new_stmt_if(Expr *cond, Stmt *then_branch, Stmt *else_branch, int line);
Stmt *new_stmt_while(Expr *cond, Stmt *body, int line);
Stmt *new_stmt_block(Block *block, int line);

Block *new_block(Decl *decls, Stmt *commands, int line);

// Utility functions
void die_alloc(void);
char *xstrdup(const char *s);

#endif
