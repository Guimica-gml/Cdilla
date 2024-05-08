#ifndef CDILLA_PARSER_H_
#define CDILLA_PARSER_H_

#include "./cdilla_lexer.h"

typedef size_t Cdilla_Expression_Id;
typedef size_t Cdilla_Code_Block_Id;

typedef enum {
    CDILLA_EXPRESSION_I32,
    CDILLA_EXPRESSION_STRING,
} Cdilla_Expression_Kind;

typedef union {
    i32 int32;
    size_t string_index;
} Cdilla_Expression_As;

typedef struct {
    Cdilla_Expression_Kind kind;
    Cdilla_Expression_As as;
} Cdilla_Expression;

typedef enum {
    CDILLA_STATEMENT_PRINT,
    CDILLA_STATEMENT_PROC_CALL,
} Cdilla_Statement_Kind;

typedef struct {
    Cdilla_Expression_Id expr_id;
} Cdilla_Statement_As_Print;

typedef struct {
    String_View name;
} Cdilla_Statement_As_Proc_Call;

typedef union {
    Cdilla_Statement_As_Print print;
    Cdilla_Statement_As_Proc_Call proc_call;
} Cdilla_Statement_As;

typedef struct {
    Cdilla_Loc loc;
    Cdilla_Statement_Kind kind;
    Cdilla_Statement_As as;
} Cdilla_Statement;

typedef struct {
    Cdilla_Statement *items;
    size_t count;
    size_t capacity;
} Cdilla_Code_Block;

typedef struct {
    String_View name;
    Cdilla_Code_Block_Id code_block_id;
} Cdilla_Procedure;

typedef struct {
    Cdilla_Expression *items;
    size_t count;
    size_t capacity;
} Cdilla_Expressions;

typedef struct {
    Cdilla_Code_Block *items;
    size_t count;
    size_t capacity;
} Cdilla_Code_Blocks;

typedef struct {
    Cdilla_Procedure *items;
    size_t count;
    size_t capacity;
} Cdilla_Procedures;

typedef struct {
    String_Builder strings;
    Cdilla_Expressions expressions;
    Cdilla_Code_Blocks code_blocks;
    Cdilla_Procedures procedures;
} Cdilla_Ast;

#define cdilla_parse_expect(lexer, ...)                                 \
    cdilla_parse_expect_impl(                                           \
        (lexer),                                                        \
        ((Cdilla_Token_Kind[]){__VA_ARGS__}),                           \
        (sizeof((Cdilla_Token_Kind[]){__VA_ARGS__}))/sizeof(Cdilla_Token_Kind))

Cdilla_Token cdilla_parse_next(Cdilla_Lexer *lexer);
Cdilla_Token cdilla_parse_expect_impl(Cdilla_Lexer *lexer, Cdilla_Token_Kind kinds[], size_t count);
Cdilla_Expression_Id cdilla_parse_expression(Cdilla_Ast *ast, Cdilla_Lexer *lexer);
Cdilla_Code_Block_Id cdilla_parse_code_block(Cdilla_Ast *ast, Cdilla_Lexer *lexer);
Cdilla_Ast cdilla_parse(Cdilla_Lexer *lexer);
void cdilla_ast_free(Cdilla_Ast *ast);
void cdilla_ast_print(Cdilla_Ast *ast);

#endif // CDILLA_PARSER_H_
