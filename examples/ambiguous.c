/**
 * @file ambiguous.c
 * @brief Ambiguity handling example using LibGLR
 * 
 * This example demonstrates GLR's ability to handle ambiguous grammars.
 * 
 * Consider the classic ambiguous grammar:
 *   expr -> expr + expr
 *   expr -> expr * expr
 *   expr -> NUMBER
 * 
 * For input "1 + 2 * 3", this grammar has two parses:
 *  1. (1 + 2) * 3 = 9
 *  2. 1 + (2 * 3) = 7
 * 
 * GLR parses both possibilities simultaneously.
 * 
 * Usage: ./ambiguous <expression>
 */

#include <glr/glr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Build ambiguous grammar
 * 
 * Creates a deliberately ambiguous grammar for demonstration.
 */
static glr_grammar_t *create_ambiguous_grammar(void) {
    glr_grammar_t *grammar = glr_grammar_create();
    if (grammar == NULL) {
        return NULL;
    }
    
    /* Add symbols */
    int tok_number = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "NUMBER");
    int tok_plus = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "+");
    int tok_multiply = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "*");
    
    int expr = glr_grammar_add_symbol(grammar, GLR_SYMBOL_NONTERMINAL, "expr");
    
    /* Ambiguous productions */
    /* expr -> expr + expr */
    glr_symbol_t *plus_body[] = {
        glr_grammar_get_symbol(grammar, expr),
        glr_grammar_get_symbol(grammar, tok_plus),
        glr_grammar_get_symbol(grammar, expr)
    };
    glr_grammar_add_production(grammar, expr, plus_body, 3);
    
    /* expr -> expr * expr */
    glr_symbol_t *mult_body[] = {
        glr_grammar_get_symbol(grammar, expr),
        glr_grammar_get_symbol(grammar, tok_multiply),
        glr_grammar_get_symbol(grammar, expr)
    };
    glr_grammar_add_production(grammar, expr, mult_body, 3);
    
    /* expr -> NUMBER */
    glr_symbol_t *num_body[] = {
        glr_grammar_get_symbol(grammar, tok_number)
    };
    glr_grammar_add_production(grammar, expr, num_body, 1);
    
    /* Set start symbol */
    glr_grammar_set_start_symbol(grammar, expr);
    
    return grammar;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <expression>\n", argv[0]);
        printf("Example: %s \"1 + 2 * 3\"\n", argv[0]);
        return 1;
    }
    
    const char *input = argv[1];
    
    printf("=== LibGLR Ambiguous Grammar Example ===\n\n");
    printf("Input: %s\n\n", input);
    printf("Grammar (ambiguous):\n");
    printf("  expr -> expr + expr\n");
    printf("  expr -> expr * expr\n");
    printf("  expr -> NUMBER\n\n");
    printf("This grammar is ambiguous: \"1 + 2 * 3\" has two parses:\n");
    printf("  1. (1 + 2) * 3 = 9\n");
    printf("  2. 1 + (2 * 3) = 7\n\n");
    
    /* Create ambiguous grammar */
    glr_grammar_t *grammar = create_ambiguous_grammar();
    if (grammar == NULL) {
        fprintf(stderr, "Failed to create grammar\n");
        return 1;
    }
    
    printf("Grammar created successfully\n");
    printf("  Symbols: %zu\n", grammar->symbol_count);
    printf("  Productions: %zu\n\n", grammar->production_count);
    
    /* Create parser */
    glr_parser_t *parser = glr_parser_create(grammar);
    if (parser == NULL) {
        fprintf(stderr, "Failed to create parser\n");
        glr_grammar_destroy(grammar);
        return 1;
    }
    
    /* Parse */
    printf("Parsing...\n");
    glr_parse_result_t result = glr_parse(parser, input, strlen(input));
    
    printf("\n=== Parse Result ===\n");
    printf("Error: %d\n", result.error);
    printf("Position: %zu\n", result.position);
    printf("Active stacks: %zu\n", glr_parser_stack_count(parser));
    printf("Input consumed: %zu/%zu\n", result.position, strlen(input));
    
    if (result.error == GLR_PARSE_SUCCESS) {
        printf("\nParse successful!\n");
        printf("Multiple stacks indicate ambiguity was detected.\n");
        
        glr_forest_t *forest = glr_parser_get_forest(parser);
        if (forest != NULL) {
            printf("Parse forest created: valid\n");
        }
    } else {
        printf("\nParse failed with error: %d\n", result.error);
    }
    
    /* Cleanup */
    glr_parser_destroy(parser);
    glr_grammar_destroy(grammar);
    
    return result.error == GLR_PARSE_SUCCESS ? 0 : 1;
}
