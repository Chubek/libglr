#ifndef GLR_PARSER_H
#define GLR_PARSER_H

#include <glr/forest.h>
#include <glr/grammar.h>
#include <glr/lexer-hooks.h>
#include <glr/reader.h>
#include <glr/reduction.h>
#include <glr/stack.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct glr_parser glr_parser_t;

#include <glr/disambiguate.h>

/**
 * @file parser.h
 * @brief GLR parser core infrastructure
 *
 * This module provides the main GLR parser implementation,
 * including the parsing loop, item set construction, and
 * ambiguity handling.
 */

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @typedef glr_parse_error_t
   * @brief Parser error codes
   */
  typedef enum
  {
    GLR_PARSE_SUCCESS,            ///< Parsing successful
    GLR_PARSE_ERROR_SYNTAX,       ///< Syntax error
    GLR_PARSE_ERROR_MEMORY,       ///< Memory allocation failed
    GLR_PARSE_ERROR_GRAMMAR,      ///< Grammar error
    GLR_PARSE_ERROR_UNRECOVERABLE ///< Unrecoverable parse error
  } glr_parse_error_t;

  /**
   * @struct glr_parse_result_t
   * @brief Result of a parse operation
   */
  typedef struct
  {
    glr_parse_error_t error; ///< Error code
    glr_forest_t *forest;    ///< Parse forest (NULL on error)
    size_t position;         ///< Parse position (input consumed)
    void *user_data;         ///< User-provided data
  } glr_parse_result_t;

  /**
   * @struct glr_parser_t
   * @brief GLR parser instance
   *
   * Contains all state needed for GLR parsing, including
   * multiple stacks for handling ambiguity.
   */
  struct glr_parser
  {
    glr_grammar_t *grammar;  ///< Grammar being parsed
    glr_stack_t **stacks;    ///< Array of stacks (one per parse path)
    size_t stack_count;      ///< Number of active stacks
    size_t stack_capacity;   ///< Stack capacity
    glr_forest_t *forest;    ///< Shared parse forest
    void **state_table;      ///< State transition table
    size_t state_table_size; ///< State table size
    glr_disambig_hook_t *disambig_hooks; ///< Disambiguation hooks
    const char *input;       ///< Current input
    size_t input_pos;        ///< Current input position
    size_t input_length;     ///< Total input length
    glr_reader_t *reader;    ///< Token reader used for UTF-16 inputs
    glr_lexer_hooks_t *lexer_hooks; ///< Optional pluggable lexer hooks
    glr_reader_token_t lookahead;   ///< Last token produced by the reader
    glr_parse_error_t error; ///< Last error
    void *user_data;         ///< User data
  };

  /**
   * @brief Create a new GLR parser
   *
   * @param grammar Grammar to parse with
   * @return Pointer to new parser, or NULL on failure
   */
  glr_parser_t *glr_parser_create (glr_grammar_t *grammar);

  /**
   * @brief Destroy a parser and free all associated memory
   *
   * @param parser Pointer to parser to destroy
   */
  void glr_parser_destroy (glr_parser_t *parser);

  /**
   * @brief Reset the parser for a new parse
   *
   * @param parser Pointer to parser
   * @return 0 on success, -1 on failure
   */
  int glr_parser_reset (glr_parser_t *parser);

  /**
   * @brief Parse input using the GLR algorithm
   *
   * Runs the full GLR parsing loop on the provided input buffer.
   * The parser is reset before each invocation.
   *
   * @param parser Pointer to an initialized parser
   * @param input  Input buffer to parse
   * @param length Length of the input buffer in bytes
   * @return A @ref glr_parse_result_t describing the outcome
   */
  glr_parse_result_t glr_parse (glr_parser_t *parser, const char *input,
                                size_t length);

  /**
   * @brief Configure lexer hooks used when parsing UTF-16 input.
   *
   * The parser does not take ownership of @p hooks.
   *
   * @param parser Pointer to parser
   * @param hooks Hook registry or NULL to disable custom tokenization
   * @return 0 on success, -1 on failure
   */
  int glr_parser_set_lexer_hooks (glr_parser_t *parser, glr_lexer_hooks_t *hooks);

  /**
   * @brief Get the lexer hooks configured on the parser.
   *
   * @param parser Pointer to parser
   * @return Hook registry or NULL
   */
  glr_lexer_hooks_t *glr_parser_get_lexer_hooks (const glr_parser_t *parser);

  /**
   * @brief Get the most recent token observed by the parser.
   *
   * @param parser Pointer to parser
   * @return Last token or NULL when no token has been produced yet
   */
  const glr_reader_token_t *glr_parser_get_last_token (const glr_parser_t *parser);

  /**
   * @brief Set user data for the parser
   *
   * @param parser Pointer to parser
   * @param data User data pointer
   */
  static inline void
  glr_parser_set_user_data (glr_parser_t *parser, void *data)
  {
    if (parser != NULL)
      {
        parser->user_data = data;
      }
  }

  /**
   * @brief Get user data from the parser
   *
   * @param parser Pointer to parser
   * @return User data pointer
   */
  static inline void *
  glr_parser_get_user_data (glr_parser_t *parser)
  {
    return parser != NULL ? parser->user_data : NULL;
  }

  /**
   * @brief Get the current error
   *
   * @param parser Pointer to parser
   * @return Error code
   */
  static inline glr_parse_error_t
  glr_parser_get_error (glr_parser_t *parser)
  {
    return parser != NULL ? parser->error : GLR_PARSE_ERROR_MEMORY;
  }

  /**
   * @brief Get the parse forest
   *
   * @param parser Pointer to parser
   * @return Parse forest, or NULL
   */
  static inline glr_forest_t *
  glr_parser_get_forest (glr_parser_t *parser)
  {
    return parser != NULL ? parser->forest : NULL;
  }

  /**
   * @brief Get the number of active stacks
   *
   * @param parser Pointer to parser
   * @return Number of stacks
   */
  static inline size_t
  glr_parser_stack_count (glr_parser_t *parser)
  {
    return parser != NULL ? parser->stack_count : 0;
  }

#ifdef __cplusplus
}
#endif

const char *glr_version (void);
const char *glr_name (void);

#endif /* GLR_PARSER_H */
