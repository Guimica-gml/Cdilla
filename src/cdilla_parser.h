#ifndef CDILLA_PARSER_H_
#define CDILLA_PARSER_H_

#include "./cdilla_lexer.h"

typedef size_t Cdilla_Expr_Id;
typedef size_t Cdilla_Code_Block_Id;

typedef enum {
    CDILLA_EXPRESSION_I32,
    CDILLA_EXPRESSION_STRING,
} Cdilla_Expr_Kind;

typedef union {
    i32 int32;
    size_t string_index;
} Cdilla_Expr_As;

typedef struct {
    Cdilla_Expr_Kind kind;
    Cdilla_Expr_As as;
} Cdilla_Expr;

typedef enum {
    CDILLA_STMT_PRINT,
    CDILLA_STMT_PROC_CALL,
} Cdilla_Stmt_Kind;

typedef struct {
    Cdilla_Expr_Id expr_id;
} Cdilla_Stmt_As_Print;

typedef struct {
    String_View name;
} Cdilla_Stmt_As_Proc_Call;

typedef union {
    Cdilla_Stmt_As_Print print;
    Cdilla_Stmt_As_Proc_Call proc_call;
} Cdilla_Stmt_As;

typedef struct {
    Cdilla_Loc loc;
    Cdilla_Stmt_Kind kind;
    Cdilla_Stmt_As as;
} Cdilla_Stmt;

typedef struct {
    String_View name;
    Cdilla_Code_Block_Id code_block_id;
} Cdilla_Proc;

typedef Da_Type(Cdilla_Stmt) Cdilla_Code_Block;
typedef Da_Type(Cdilla_Expr) Cdilla_Exprs;
typedef Da_Type(Cdilla_Code_Block) Cdilla_Code_Blocks;
typedef Da_Type(Cdilla_Proc) Cdilla_Procs;

typedef struct {
    String_Builder strings;
    Cdilla_Exprs exprs;
    Cdilla_Code_Blocks code_blocks;
    Cdilla_Procs procs;
} Cdilla_Ast;

#define cdilla_parse_expect(lexer, ...)                                 \
    cdilla_parse_expect_impl(                                           \
        (lexer),                                                        \
        ((Cdilla_Token_Kind[]){__VA_ARGS__}),                           \
        (sizeof((Cdilla_Token_Kind[]){__VA_ARGS__}))/sizeof(Cdilla_Token_Kind))

Cdilla_Token cdilla_parse_next(Cdilla_Lexer *lexer);
Cdilla_Token cdilla_parse_expect_impl(Cdilla_Lexer *lexer, Cdilla_Token_Kind kinds[], size_t count);
Cdilla_Expr_Id cdilla_parse_expression(Cdilla_Ast *ast, Cdilla_Lexer *lexer);
Cdilla_Code_Block_Id cdilla_parse_code_block(Cdilla_Ast *ast, Cdilla_Lexer *lexer);
Cdilla_Ast cdilla_parse(Cdilla_Lexer *lexer);
void cdilla_ast_free(Cdilla_Ast *ast);
void cdilla_ast_print(Cdilla_Ast *ast);

#endif // CDILLA_PARSER_H_
