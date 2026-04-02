/**
 * @file calc.c
 * @brief Calculator example using LibGLR
 * 
 * This example demonstrates a simple expression parser for an arithmetic
 * calculator. It parses expressions with addition, subtraction, multiplication,
 * and division, respecting operator precedence.
 * 
 * Grammar:
 *   expression -> term { (+|-) term }
 *   term       -> factor { (*|/) factor }
 *   factor     -> NUMBER | ( expression )
 * 
 * Usage: ./calc <expression>
 * Example: ./calc "1 + 2 * 3"
 */

#include <glr/glr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * @brief Token type enumeration
 */
typedef enum {
    TOK_NUMBER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULTIPLY,
    TOK_DIVIDE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_END
} tokentype_t;

/**
 * @brief Token structure
 */
typedef struct {
    tokentype_t type;
    double value;
    char *name;
} token_t;

/**
 * @brief Lexer for calculator expressions
 */
typedef struct {
    const char *input;
    size_t pos;
    token_t current;
} lexer_t;

/**
 * @brief Initialize lexer with input string
 */
static void lexer_init(lexer_t *lexer, const char *input) {
    lexer->input = input;
    lexer->pos = 0;
    lexer_next(lexer);
}

/**
 * @brief Get next token
 */
static void lexer_next(lexer_t *lexer) {
    while (isspace(lexer->input[lexer->pos])) {
        lexer->pos++;
    }
    
    char c = lexer->input[lexer->pos];
    
    if (c == '\0') {
        lexer->current.type = TOK_END;
        return;
    }
    
    if (isdigit(c) || c == '.') {
        lexer->current.type = TOK_NUMBER;
        lexer->current.value = strtod(&lexer->input[lexer->pos], NULL);
        while (isdigit(lexer->input[++lexer->pos]) || lexer->input[lexer->pos] == '.') {}
        return;
    }
    
    switch (c) {
        case '+': lexer->current.type = TOK_PLUS; break;
        case '-': lexer->current.type = TOK_MINUS; break;
        case '*': lexer->current.type = TOK_MULTIPLY; break;
        case '/': lexer->current.type = TOK_DIVIDE; break;
        case '(': lexer->current.type = TOK_LPAREN; break;
        case ')': lexer->current.type = TOK_RPAREN; break;
        default:
            lexer->current.type = TOK_END;
            break;
    }
    lexer->pos++;
}

/**
 * @brief Get token name
 */
static const char *token_name(tokentype_t type) {
    switch (type) {
        case TOK_NUMBER: return "NUMBER";
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_MULTIPLY: return "*";
        case TOK_DIVIDE: return "/";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_END: return "END";
        default: return "?";
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <expression>\n", argv[0]);
        printf("Example: %s \"1 + 2 * 3\"\n", argv[0]);
        return 1;
    }
    
    const char *input = argv[1];
    
    printf("Parsing: %s\n\n", input);
    
    // Create grammar
    glr_grammar_t *grammar = glr_grammar_create();
    if (grammar == NULL) {
        fprintf(stderr, "Failed to create grammar\n");
        return 1;
    }
    
    /* Add tokens as symbols */
    int tok_number = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "NUMBER");
    int tok_plus = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "+");
    int tok_minus = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "-");
    int tok_multiply = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "*");
    int tok_divide = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "/");
    int tok_lparen = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, "(");
    int tok_rparen = glr_grammar_add_symbol(grammar, GLR_SYMBOL_TERMINAL, ")");
    
    int expr = glr_grammar_add_symbol(grammar, GLR_SYMBOL_NONTERMINAL, "expression");
    int term = glr_grammar_add_symbol(grammar, GLR_SYMBOL_NONTERMINAL, "term");
    int factor = glr_grammar_add_symbol(grammar, GLR_SYMBOL_NONTERMINAL, "factor");
    
    /* Create productions */
    /* expression -> term { (+|-) term } */
    glr_symbol_t *expr_body[] = {
        glr_grammar_get_symbol(grammar, term)
    };
    glr_grammar_add_production(grammar, expr, expr_body, 1);
    
    /* term -> factor { (*|/) factor } */
    glr_symbol_t *term_body[] = {
        glr_grammar_get_symbol(grammar, factor)
    };
    glr_grammar_add_production(grammar, term, term_body, 1);
    
    /* factor -> NUMBER */
    glr_symbol_t *factor1[] = {
        glr_grammar_get_symbol(grammar, tok_number)
    };
    glr_grammar_add_production(grammar, factor, factor1, 1);
    
    /* factor -> ( expression ) */
    glr_symbol_t *factor2[] = {
        glr_grammar_get_symbol(grammar, tok_lparen),
        glr_grammar_get_symbol(grammar, expr),
        glr_grammar_get_symbol(grammar, tok_rparen)
    };
    glr_grammar_add_production(grammar, factor, factor2, 3);
    
    /* Set start symbol */
    glr_grammar_set_start_symbol(grammar, expr);
    
    /* Create parser */
    glr_parser_t *parser = glr_parser_create(grammar);
    if (parser == NULL) {
        fprintf(stderr, "Failed to create parser\n");
        glr_grammar_destroy(grammar);
        return 1;
    }
    
    /* Parse */
    glr_parse_result_t result = glr_parse(parser, input, strlen(input));
    
    printf("Parse result:\n");
    printf("  Error: %d\n", result.error);
    printf("  Position: %zu\n", result.position);
    printf("  Stacks: %zu\n", glr_parser_stack_count(parser));
    
    if (result.error == GLR_PARSE_SUCCESS && result.forest != NULL) {
        printf("  Parse forest: valid\n");
    } else {
        printf("  Parse failed\n");
    }
    
    /* Cleanup */
    glr_parser_destroy(parser);
    glr_grammar_destroy(grammar);
    
    return result.error == GLR_PARSE_SUCCESS ? 0 : 1;
}
