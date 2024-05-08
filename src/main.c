#include "./utils.h"
#include "./cdilla_lexer.h"
#include "./cdilla_parser.h"

void print_usage(FILE *stream, const char *program) {
    fprintf(stream, "Usage: %s <filepath>\n", program);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Error: expected source code filepath\n");
        print_usage(stderr, argv[0]);
        exit(1);
    }

    const char *source_filepath = argv[1];
    String_Builder content = {0};

    Errno err = read_file(source_filepath, &content);
    if (err) {
        fprintf(
            stderr, "Error: couldn't read file %s: %s\n",
            source_filepath, strerror(err));
        exit(1);
    }

    String_View code = sv_from_sb(&content);
    Cdilla_Lexer lexer = cdilla_lexer_new(code, source_filepath);
    Cdilla_Ast ast = cdilla_parse(&lexer);

    cdilla_ast_print(&ast);
    cdilla_ast_free(&ast);

    sb_free(&content);
    return 0;
}
