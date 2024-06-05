#include "./cdilla_interpreter.h"

// TODO(nic): linear searches, deal with them??? (Hash Tables)
// TODO(nic): start thinking of a better way to report errors

#define CDILLA_MAIN_PROC "main"

typedef struct {
    String_View name;
    i64 value;
} Cdilla_Variable;

typedef Da_Type(Cdilla_Variable) Cdilla_Scope;

Cdilla_Variable *cdilla_get_var(Cdilla_Scope *scope, String_View var_name) {
    for (size_t i = 0; i < da_count(scope); ++i) {
        if (sv_equals(scope->items[i].name, var_name)) {
            return &scope->items[i];
        }
    }
    return NULL;
}

Cdilla_Proc *cdilla_get_proc(Cdilla_Ast *ast, String_View proc_name) {
    for (size_t i = 0; i < da_count(&ast->procs); ++i) {
        if (sv_equals(ast->procs.items[i].name, proc_name)) {
            return &ast->procs.items[i];
        }
    }
    return NULL;
}

i64 cdilla_interpret_expr(Cdilla_Ast *ast, Cdilla_Scope *scope, Cdilla_Expr_Id expr_id) {
    Cdilla_Expr *expr = &ast->exprs.items[expr_id];
    switch (expr->kind) {
    case CDILLA_EXPR_I64: {
        return expr->as.int64;
    } break;
    case CDILLA_EXPR_STRING: {
        return (i64)&ast->strings.items[expr->as.string_index];
    } break;
    case CDILLA_EXPR_IDENTIFIER: {
        String_View var_name = expr->as.ident;
        Cdilla_Variable *var = cdilla_get_var(scope, var_name);
        if (var == NULL) {
            fprintf(
                stderr,
                CDILLA_LOC_FMT": Error: no '"SV_FMT"' variable found in scope\n",
                CDILLA_LOC_ARG(expr->loc),
                SV_ARG(var_name));
            exit(1);
        }
        return var->value;
    } break;
    }
    PANIC(SOURCE_LOC, "unreachable");
}

void cdilla_interpret_proc(Cdilla_Ast *ast, Cdilla_Proc *proc) {
    Cdilla_Scope scope = {0};
    Cdilla_Code_Block *code_block = &ast->code_blocks.items[proc->code_block_id];
    for (size_t i = 0; i < da_count(code_block); ++i) {
        Cdilla_Stmt *stmt = &code_block->items[i];
        switch (stmt->kind) {
        case CDILLA_STMT_PRINT: {
            i64 value = cdilla_interpret_expr(ast, &scope, stmt->as.print.expr_id);
            printf("%ld\n", value);
        } break;
        case CDILLA_STMT_PROC_CALL: {
            String_View proc_name = stmt->as.proc_call.name;
            Cdilla_Proc *proc_to_call = cdilla_get_proc(ast, proc_name);
            if (proc_to_call == NULL) {
                fprintf(
                    stderr,
                    CDILLA_LOC_FMT": Error: no '"SV_FMT"' procedure found in source code\n",
                    CDILLA_LOC_ARG(stmt->loc),
                    SV_ARG(proc_name));
                exit(1);
            }
            cdilla_interpret_proc(ast, proc_to_call);
        } break;
        case CDILLA_STMT_LET: {
            Cdilla_Stmt_As_Let *let = &stmt->as.let;
            i64 value = cdilla_interpret_expr(ast, &scope, let->expr_id);
            Cdilla_Variable var = { let->var_name, value };
            da_append(&scope, var);
        } break;
        }
    }
    da_free(&scope);
}

void cdilla_interpret(Cdilla_Ast *ast) {
    Cdilla_Proc *main_proc = cdilla_get_proc(ast, sv_from_cstr(CDILLA_MAIN_PROC));
    if (main_proc == NULL) {
        fprintf(
            stderr,
            "Error: no '%s' procedure found in source code\n",
            CDILLA_MAIN_PROC);
        exit(1);
    }
    cdilla_interpret_proc(ast, main_proc);
}
