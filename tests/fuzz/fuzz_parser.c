/**
 * AFL Fuzzing harness for parser operations
 */

#include <glr/parser.h>
#include <glr/grammar.h>
#include <glr/reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_INPUT_SIZE 4096

static glr_grammar_t* create_simple_grammar(void) {
    glr_grammar_t* grammar = glr_grammar_create();
    if (!grammar) return NULL;
    
    int expr = glr_grammar_add_nonterminal(grammar, "Expr");
    int num = glr_grammar_add_terminal(grammar, "NUM");
    int plus = glr_grammar_add_terminal(grammar, "PLUS");
    
    int syms1[] = {num};
    glr_grammar_add_production(grammar, expr, syms1, 1);
    
    int syms2[] = {expr, plus, expr};
    glr_grammar_add_production(grammar, expr, syms2, 3);
    
    glr_grammar_finalize(grammar);
    return grammar;
}

int main(int argc, char** argv) {
    uint8_t buffer[MAX_INPUT_SIZE];
    size_t len = 0;

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
#endif

    glr_grammar_t* grammar = create_simple_grammar();
    if (!grammar) return 1;

    while (__AFL_LOOP(1000)) {
        len = fread(buffer, 1, MAX_INPUT_SIZE, stdin);
        if (len == 0) continue;

        glr_reader_t* reader = glr_reader_create_from_string((const char*)buffer, len);
        if (!reader) continue;

        glr_parser_t* parser = glr_parser_create(grammar);
        if (parser) {
            glr_forest_t* forest = NULL;
            glr_parser_parse(parser, reader, &forest);
            
            if (forest) {
                glr_forest_destroy(forest);
            }
            
            glr_parser_destroy(parser);
        }

        glr_reader_destroy(reader);
    }

    glr_grammar_destroy(grammar);
    return 0;
}
