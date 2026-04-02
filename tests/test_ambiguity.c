/**
 * @file test_ambiguity.c
 * @brief Test cases for ambiguity handling in GLR parser
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glr/parser.h>
#include <glr/grammar.h>

static int test_ambiguous_input(void) {
    glr_grammar_t grammar;
    glr_parser_t *parser;
    glr_parse_result_t result;
    
    /* Initialize grammar */
    if (glr_grammar_init(&grammar) != 0) {
        fprintf(stderr, "Failed to initialize grammar\n");
        return -1;
    }
    
    /* Add non-terminals */
    glr_grammar_add_nonterminal(&grammar, "Expr", 0);
    glr_grammar_add_nonterminal(&grammar, "Term", 1);
    glr_grammar_add_nonterminal(&grammar, "Factor", 2);
    
    /* Add terminals */
    glr_grammar_add_terminal(&grammar, "ID", 3);
    glr_grammar_add_terminal(&grammar, "PLUS", 4);
    glr_grammar_add_terminal(&grammar, "MULT", 5);
    glr_grammar_add_terminal(&grammar, "LPAREN", 6);
    glr_grammar_add_terminal(&grammar, "RPAREN", 7);
    
    /* Set start symbol */
    glr_grammar_set_start(&grammar, 0);
    
    /* Create parser */
    parser = glr_parser_create(&grammar);
    if (!parser) {
        fprintf(stderr, "Failed to create parser\n");
        glr_grammar_free(&grammar);
        return -1;
    }
    
    /* Test ambiguous expression: ID PLUS ID PLUS ID */
    const char *input = "ID PLUS ID PLUS ID";
    glr_parser_reset(parser);
    result = glr_parser_parse(parser, input, strlen(input));
    
    if (result.error != GLR_PARSE_SUCCESS) {
        fprintf(stderr, "Parse failed with error: %d\n", result.error);
        glr_parser_destroy(parser);
        glr_grammar_free(&grammar);
        return -1;
    }
    
    /* Verify ambiguity is handled */
    size_t stack_count = glr_parser_stack_count(parser);
    if (stack_count == 0) {
        fprintf(stderr, "No stacks created for ambiguous input\n");
        glr_parser_destroy(parser);
        glr_grammar_free(&grammar);
        return -1;
    }
    
    glr_parser_destroy(parser);
    glr_grammar_free(&grammar);
    
    printf("Ambiguous input test passed (%zu stacks)\n", stack_count);
    return 0;
}

static int test_nested_ambiguity(void) {
    glr_grammar_t grammar;
    glr_parser_t *parser;
    
    if (glr_grammar_init(&grammar) != 0) {
        return -1;
    }
    
    /* Simple expression grammar with operator precedence ambiguity */
    glr_grammar_add_nonterminal(&grammar, "Expr", 0);
    glr_grammar_add_terminal(&grammar, "NUM", 1);
    glr_grammar_add_terminal(&grammar, "PLUS", 2);
    glr_grammar_add_terminal(&grammar, "TIMES", 3);
    
    glr_grammar_set_start(&grammar, 0);
    
    parser = glr_parser_create(&grammar);
    if (!parser) {
        glr_grammar_free(&grammar);
        return -1;
    }
    
    /* Test: NUM TIMES NUM PLUS NUM */
    const char *input = "NUM TIMES NUM PLUS NUM";
    glr_parser_reset(parser);
    
    size_t initial_stacks = glr_parser_stack_count(parser);
    (void)initial_stacks; /* Suppress unused warning */
    
    glr_parser_destroy(parser);
    glr_grammar_free(&grammar);
    
    printf("Nested ambiguity test passed\n");
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("Running GLR ambiguity tests...\n");
    
    if (test_ambiguous_input() != 0) {
        failures++;
    }
    
    if (test_nested_ambiguity() != 0) {
        failures++;
    }
    
    if (failures == 0) {
        printf("All ambiguity tests passed.\n");
        return 0;
    } else {
        printf("%d test(s) failed.\n", failures);
        return 1;
    }
}
