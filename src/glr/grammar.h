#ifndef LIBGLR_GRAMMAR_H
#define LIBGLR_GRAMMAR_H

#include <stddef.h>

/**
 * @file grammar.h
 * @brief Grammar definition structures for LibGLR
 */

/**
 * @brief Non-terminal symbol identifier
 */
typedef struct glr_nt_symbol {
    char *name;
    int id;
} glr_nt_symbol_t;

/**
 * @brief Terminal symbol identifier
 */
typedef struct glr_tt_symbol {
    char *name;
    int id;
} glr_tt_symbol_t;

/**
 * @brief Grammar production rule
 */
typedef struct glr_production {
    glr_nt_symbol_t *nt;
    glr_tt_symbol_t **terms;
    size_t term_count;
    size_t terms_capacity;
} glr_production_t;

/**
 * @brief Complete grammar definition
 */
typedef struct glr_grammar {
    glr_nt_symbol_t *nonterminals;
    size_t nt_count;
    size_t nt_capacity;
    
    glr_tt_symbol_t *terminals;
    size_t term_count;
    size_t term_capacity;
    
    glr_production_t *productions;
    size_t production_count;
    size_t production_capacity;
    
    int start_symbol;
} glr_grammar_t;

/**
 * @brief Initialize an empty grammar structure
 * @param grammar Pointer to grammar structure to initialize
 * @return 0 on success, -1 on failure
 */
int glr_grammar_init(glr_grammar_t *grammar);

/**
 * @brief Free all memory associated with a grammar
 * @param grammar Pointer to grammar structure to free
 */
void glr_grammar_free(glr_grammar_t *grammar);

/**
 * @brief Add a non-terminal symbol to the grammar
 * @param grammar Pointer to grammar structure
 * @param name Symbol name
 * @param id Symbol identifier
 * @return 0 on success, -1 on failure
 */
int glr_grammar_add_nonterminal(glr_grammar_t *grammar, const char *name, int id);

/**
 * @brief Add a terminal symbol to the grammar
 * @param grammar Pointer to grammar structure
 * @param name Symbol name
 * @param id Symbol identifier
 * @return 0 on success, -1 on failure
 */
int glr_grammar_add_terminal(glr_grammar_t *grammar, const char *name, int id);

/**
 * @brief Add a production rule to the grammar
 * @param grammar Pointer to grammar structure
 * @param nt Non-terminal on LHS
 * @param terms Array of terminals on RHS
 * @param term_count Number of terminals
 * @return 0 on success, -1 on failure
 */
int glr_grammar_add_production(glr_grammar_t *grammar, glr_nt_symbol_t *nt, 
                                glr_tt_symbol_t **terms, size_t term_count);

/**
 * @brief Set the start symbol for the grammar
 * @param grammar Pointer to grammar structure
 * @param id Identifier of start non-terminal
 * @return 0 on success, -1 on failure
 */
int glr_grammar_set_start(glr_grammar_t *grammar, int id);

/**
 * @brief Get a non-terminal by ID
 * @param grammar Pointer to grammar structure
 * @param id Non-terminal identifier
 * @return Pointer to non-terminal or NULL if not found
 */
glr_nt_symbol_t *glr_grammar_get_nonterminal(glr_grammar_t *grammar, int id);

/**
 * @brief Get a terminal by ID
 * @param grammar Pointer to grammar structure
 * @param id Terminal identifier
 * @return Pointer to terminal or NULL if not found
 */
glr_tt_symbol_t *glr_grammar_get_terminal(glr_grammar_t *grammar, int id);

#endif /* LIBGLR_GRAMMAR_H */
