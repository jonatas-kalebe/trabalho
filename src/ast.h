#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <string.h>

typedef enum {
    TYPE_INT,
    TYPE_CAR
} Type;

enum {
    CAT_NONE = 0,
    CAT_GLOBAL,
    CAT_PARAM,
    CAT_LOCAL
};

typedef struct {
    int  category;
    int  position;
    Type type;
    int  is_array;
} VarRef;

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

typedef enum {
    EX_INT,
    EX_CHAR,
    EX_VAR,
    EX_ARRAY,
    EX_CALL,
    EX_ASSIGN,
    EX_BINARY,
    EX_UNARY
} ExprKind;

typedef struct Expr Expr;
typedef struct Arg  Arg;

struct Expr {
    ExprKind kind;
    int      line;
    Type     inferred_type;
    int      is_array_result;
    union {
        int int_value;
        int char_value;
        struct { char *name; VarRef ref; } var;
        struct { char *name; Expr *index; VarRef ref; } arr;
        struct { char *name; Arg *args; int argc; } call;
        struct {
            char  *name;
            Expr  *index;
            Expr  *value;
            VarRef ref;
        } assign;
        struct { OpKind op; Expr *left, *right; } bin;
        struct { OpKind op; Expr *operand; } un;
    } as;
};

struct Arg {
    Expr *expr;
    Arg  *next;
};

typedef struct Decl {
    char *name;
    Type  type;
    int   is_array;
    int   array_size;
    int   line;
    int   category;
    int   position;
    struct Decl *next;
} Decl;

typedef struct Param {
    char *name;
    Type  type;
    int   is_array;
    int   line;
    int   index;
    struct Param *next;
} Param;

typedef struct Block Block;

typedef enum {
    ST_EMPTY,
    ST_EXPR,
    ST_RETURN,
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
    int      line;
    struct Stmt *next;
    union {
        Expr *expr;
        struct { char *name; Expr *index; VarRef ref; } read;
        struct { char *text; char *label; } wstr;
        struct { Expr *cond; struct Stmt *then_branch;
                 struct Stmt *else_branch; } if_s;
        struct { Expr *cond; struct Stmt *body; } while_s;
        Block *block;
    } as;
} Stmt;

struct Block {
    Decl *decls;
    Stmt *commands;
    int   line;
};

typedef struct Func {
    char  *name;
    Param *params;
    int    param_count;
    Type   return_type;
    Block *body;
    int    line;
    struct Func *next;
} Func;

typedef struct {
    Decl *globals;
    Func *functions;
    Block *main_block;
} Program;

Expr *new_expr_int   (int value, int line);
Expr *new_expr_char  (int value, int line);
Expr *new_expr_var   (char *name, int line);
Expr *new_expr_array (char *name, Expr *index, int line);
Expr *new_expr_call  (char *name, Arg *args, int line);
Expr *new_expr_assign(char *name, Expr *index, Expr *value, int line);
Expr *new_expr_binary(OpKind op, Expr *left, Expr *right, int line);
Expr *new_expr_unary (OpKind op, Expr *operand, int line);
Arg  *new_arg        (Expr *expr);

Decl  *new_decl  (char *name, Type type, int is_array, int array_size, int line);
Param *new_param (char *name, Type type, int is_array, int line);
Block *new_block (Decl *decls, Stmt *commands, int line);
Func  *new_func  (char *name, Param *params, Type return_type, Block *body, int line);

Stmt *new_stmt_empty     (int line);
Stmt *new_stmt_expr      (Expr *expr, int line);
Stmt *new_stmt_return    (Expr *expr, int line);
Stmt *new_stmt_read      (char *name, Expr *index, int line);
Stmt *new_stmt_write_expr(Expr *expr, int line);
Stmt *new_stmt_write_str (char *text, int line);
Stmt *new_stmt_newline   (int line);
Stmt *new_stmt_if        (Expr *cond, Stmt *then_b, Stmt *else_b, int line);
Stmt *new_stmt_while     (Expr *cond, Stmt *body, int line);
Stmt *new_stmt_block     (Block *block, int line);

void  die_alloc(void);
char *xstrdup(const char *s);

#endif
