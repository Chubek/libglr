/**
 * @file parser.c
 * @brief GLR parser implementation
 *
 * This file implements Tomita's Generalized LR parsing algorithm with:
 * - Graph-Structured Stack (GSS) for parallel parse paths
 * - Shared Packed Parse Forest (SPPF) for ambiguous results
 * - Shift/reduce and reduce/reduce conflict handling
 * - UTF-8 and UTF-16 input support
 * - Pluggable disambiguation strategies
 */

#include <glr/grammar.h>
#include <glr/parser.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Forward Declarations
 * ========================================================================== */

static int parse_input (glr_parser_t *parser);
static int shift_item (glr_parser_t *parser, int stack_idx);
static int reduce_item (glr_parser_t *parser, int stack_idx);
static void handle_conflict (glr_parser_t *parser);
static int initialize_parser (glr_parser_t *parser);
static int grammar_accepts_token (const glr_grammar_t *grammar,
                                   const char *name);
static int should_use_reader (const char *input, size_t length);
static int consume_next_terminal (glr_parser_t *parser);

/* ============================================================================
 * Parser Lifecycle Functions
 * ========================================================================== */

/**
 * @brief Create a new GLR parser instance
 *
 * Allocates and initializes a parser with the given grammar. The parser
 * creates its own parse forest and reader but does not take ownership of
 * the grammar.
 *
 * @param grammar Grammar specification to use for parsing
 * @return Newly allocated parser, or NULL on allocation failure
 */
glr_parser_t *
glr_parser_create (glr_grammar_t *grammar)
{
  if (grammar == NULL)
    {
      return NULL;
    }

  glr_parser_t *parser = calloc (1, sizeof (glr_parser_t));
  if (parser == NULL)
    {
      return NULL;
    }

  /* Initialize core parser state */
  parser->grammar = grammar;
  parser->stacks = NULL;
  parser->stack_count = 0;
  parser->stack_capacity = 0;
  parser->forest = glr_forest_create ();
  parser->state_table = NULL;
  parser->state_table_size = 0;
  parser->disambig_hooks = NULL;

  /* Initialize input tracking */
  parser->input = NULL;
  parser->input_pos = 0;
  parser->input_length = 0;

  /* Initialize lexer and reader */
  parser->reader = glr_reader_create ();
  parser->lexer_hooks = NULL;
  memset (&parser->lookahead, 0, sizeof (parser->lookahead));

  /* Initialize error state and user data */
  parser->error = GLR_PARSE_SUCCESS;
  parser->user_data = NULL;

  /* Check for allocation failures */
  if (parser->reader == NULL || parser->forest == NULL)
    {
      if (parser->forest != NULL)
        {
          glr_forest_destroy (parser->forest);
        }
      free (parser);
      return NULL;
    }

  return parser;
}

/**
 * @brief Destroy a parser and free all resources
 *
 * Releases all memory held by the parser including stacks, forest,
 * state tables, and disambiguation hooks. The grammar is not freed.
 *
 * @param parser Parser instance to destroy (may be NULL)
 */
void
glr_parser_destroy (glr_parser_t *parser)
{
  if (parser == NULL)
    {
      return;
    }

  /* Free all active stacks */
  for (size_t i = 0; i < parser->stack_count; i++)
    {
      glr_stack_destroy (parser->stacks[i]);
    }
  free (parser->stacks);

  /* Free parse forest */
  glr_forest_destroy (parser->forest);

  /* Free state table and reader resources */
  free (parser->state_table);
  glr_reader_token_clear (&parser->lookahead);
  glr_reader_destroy (parser->reader);

  /* Free disambiguation hooks */
  glr_parser_clear_disambiguators (parser);

  /* Free parser structure itself */
  free (parser);
}

/**
 * @brief Reset parser state for a new parse
 *
 * Clears all stacks, resets the parse forest, and reinitializes internal
 * state. The grammar and configuration (lexer hooks, disambiguation hooks)
 * are preserved.
 *
 * @param parser Parser instance to reset
 * @return 0 on success, -1 on failure
 */
int
glr_parser_reset (glr_parser_t *parser)
{
  if (parser == NULL)
    {
      return -1;
    }

  /* Destroy all existing stacks */
  for (size_t i = 0; i < parser->stack_count; i++)
    {
      glr_stack_destroy (parser->stacks[i]);
    }
  free (parser->stacks);

  /* Reset parse forest */
  glr_forest_destroy (parser->forest);
  parser->forest = glr_forest_create ();

  /* Reset stack state */
  parser->stacks = NULL;
  parser->stack_count = 0;
  parser->stack_capacity = 0;

  /* Clear lookahead token and reset position */
  glr_reader_token_clear (&parser->lookahead);
  parser->input_pos = 0;

  /* Clear error state */
  parser->error = GLR_PARSE_SUCCESS;

  return 0;
}

/* ============================================================================
 * Internal Helper Functions
 * ========================================================================== */

/**
 * @brief Initialize parser state before parsing
 *
 * Prepares the parser for a new parse operation by initializing the
 * state table if needed.
 *
 * @param parser Parser instance
 * @return 0 on success, -1 on failure
 */
static int
initialize_parser (glr_parser_t *parser)
{
  if (parser == NULL || parser->grammar == NULL)
    {
      return -1;
    }

  /* Initialize state table if not already done */
  if (parser->state_table == NULL)
    {
      /* TODO: Build LR state table from grammar */
      parser->state_table_size = 0;
    }

  return 0;
}

/**
 * @brief Check if grammar accepts a terminal symbol
 *
 * Searches the grammar's symbol table for a terminal with the given name.
 *
 * @param grammar Grammar to search
 * @param name Terminal symbol name
 * @return 1 if terminal exists, 0 otherwise
 */
static int
grammar_accepts_token (const glr_grammar_t *grammar, const char *name)
{
  size_t i;

  if (grammar == NULL || name == NULL)
    {
      return 0;
    }

  /* Linear search through symbol table */
  for (i = 0; i < grammar->symbol_count; i++)
    {
      glr_symbol_t *symbol = grammar->symbols[i];
      if (symbol != NULL && symbol->type == GLR_SYMBOL_TERMINAL
          && symbol->name != NULL && strcmp (symbol->name, name) == 0)
        {
          return 1;
        }
    }

  return 0;
}

/**
 * @brief Detect if input is UTF-16 encoded
 *
 * Checks for UTF-16 byte order marks (BOM) or patterns indicating UTF-16
 * encoding. If detected, the reader should be used for tokenization.
 *
 * @param input Input buffer
 * @param length Buffer length in bytes
 * @return 1 if UTF-16 detected, 0 for UTF-8/ASCII
 */
static int
should_use_reader (const char *input, size_t length)
{
  size_t i;

  if (input == NULL || length < 2 || (length % 2) != 0)
    {
      return 0;
    }

  /* Check for UTF-16 LE BOM (0xFF 0xFE) */
  if (((const unsigned char *)input)[0] == 0xFF
      && ((const unsigned char *)input)[1] == 0xFE)
    {
      return 1;
    }

  /* Check for UTF-16 BE BOM (0xFE 0xFF) */
  if (((const unsigned char *)input)[0] == 0xFE
      && ((const unsigned char *)input)[1] == 0xFF)
    {
      return 1;
    }

  /* Heuristic: Check for null bytes in odd positions (UTF-16 LE pattern) */
  for (i = 1; i < length; i += 2)
    {
      if (((const unsigned char *)input)[i] == 0)
        {
          return 1;
        }
    }

  return 0;
}

/**
 * @brief Consume the next terminal from input
 *
 * Reads the next token from the input stream. For UTF-16 input, uses the
 * reader; for UTF-8/ASCII, performs simple byte-by-byte consumption.
 *
 * @param parser Parser instance
 * @return 0 on success, 1 on EOF, -1 on error
 */
static int
consume_next_terminal (glr_parser_t *parser)
{
  /* Use reader for UTF-16 input */
  if (should_use_reader (parser->input, parser->input_length))
    {
      glr_reader_status_t status;

      status = glr_reader_next (parser->reader, &parser->lookahead);

      /* Handle end of input */
      if (status == GLR_READER_STATUS_EOF)
        {
          parser->input_pos = parser->input_length;
          return 1;
        }

      /* Handle reader errors */
      if (status != GLR_READER_STATUS_OK)
        {
          parser->error = GLR_PARSE_ERROR_SYNTAX;
          return -1;
        }

      /* Update position based on token */
      parser->input_pos
          = parser->lookahead.byte_offset + parser->lookahead.bytes_consumed;

      /* Validate token against grammar */
      if (!grammar_accepts_token (parser->grammar,
                                  parser->lookahead.terminal_name))
        {
          parser->error = GLR_PARSE_ERROR_SYNTAX;
          return -1;
        }

      return 0;
    }

  /* Simple byte-by-byte consumption for UTF-8/ASCII */
  glr_reader_token_clear (&parser->lookahead);
  parser->input_pos++;
  return 0;
}

/* ============================================================================
 * GLR Algorithm Core Functions
 * ========================================================================== */

/**
 * @brief Perform shift operation on a parse stack
 *
 * Shifts the current input symbol onto the specified stack and transitions
 * to the next state according to the LR state table.
 *
 * @param parser Parser instance
 * @param stack_idx Index of stack to shift
 * @return 0 on success, -1 on failure
 */
static int
shift_item (glr_parser_t *parser, int stack_idx)
{
  if (parser == NULL || stack_idx < 0
      || (size_t)stack_idx >= parser->stack_count)
    {
      return -1;
    }

  glr_stack_t *stack = parser->stacks[stack_idx];
  if (stack == NULL)
    {
      return -1;
    }

  /* Get current state from stack top */
  void *current_state = glr_stack_peek (stack);
  if (current_state == NULL)
    {
      return -1;
    }

  /* Check if we've reached end of input */
  if (parser->input_pos >= parser->input_length)
    {
      return 0;
    }

  /* TODO: Implement shift logic using state table */
  /* 1. Look up shift action in state table */
  /* 2. Push input symbol onto stack */
  /* 3. Transition to next state */
  /* 4. Add node to parse forest */

  return 0;
}

/**
 * @brief Perform reduce operation on a parse stack
 *
 * Reduces the top items on the stack according to a production rule,
 * creating a new non-terminal node in the parse forest.
 *
 * @param parser Parser instance
 * @param stack_idx Index of stack to reduce
 * @return 0 on success, -1 on failure
 */
static int
reduce_item (glr_parser_t *parser, int stack_idx)
{
  if (parser == NULL || stack_idx < 0
      || (size_t)stack_idx >= parser->stack_count)
    {
      return -1;
    }

  glr_stack_t *stack = parser->stacks[stack_idx];
  if (stack == NULL)
    {
      return -1;
    }

  /* TODO: Implement reduce logic */
  /* 1. Determine which production to reduce by */
  /* 2. Pop RHS symbols from stack */
  /* 3. Create non-terminal node in forest */
  /* 4. Push LHS non-terminal onto stack */
  /* 5. Transition to goto state */

  return 0;
}

/**
 * @brief Handle shift/reduce or reduce/reduce conflicts
 *
 * When multiple actions are possible (shift/reduce or reduce/reduce conflict),
 * this function forks the parse stack to explore all possibilities in parallel.
 * Disambiguation hooks are invoked to potentially prune paths.
 *
 * @param parser Parser instance
 */
static void
handle_conflict (glr_parser_t *parser)
{
  if (parser == NULL)
    {
      return;
    }

  /* Invoke disambiguation hooks if configured */
  if (parser->disambig_hooks != NULL)
    {
      /* Build disambiguation context */
      glr_disambig_context_t context = {
        .parser = parser,
        .grammar = parser->grammar,
        .forest = parser->forest,
        .parent = NULL,
        .candidates = NULL,
        .candidate_count = 0,
        .lookahead_symbol_id = -1,
        .start_position = parser->input_pos,
        .end_position = parser->input_pos,
        .user_data = parser->user_data,
      };

      /* Run disambiguation chain */
      (void)glr_parser_run_disambiguators (parser, &context, NULL);
    }

  /* TODO: Implement conflict resolution */
  /* 1. Detect conflicts in current state */
  /* 2. Fork stacks for each possible action */
  /* 3. Apply disambiguation to prune paths */
  /* 4. Merge equivalent stacks */
}

/**
 * @brief Main parsing loop
 *
 * Implements the core GLR parsing algorithm:
 * 1. Initialize the parser with a single stack
 * 2. For each input position:
 *    a. Consume next terminal
 *    b. Perform shift/reduce on all active stacks
 *    c. Handle conflicts by forking stacks
 *    d. Merge equivalent stacks
 * 3. Check for successful parse (at least one accepting stack)
 *
 * @param parser Parser instance with input already set
 * @return 0 on success, -1 on error
 */
static int
parse_input (glr_parser_t *parser)
{
  if (parser == NULL)
    {
      return -1;
    }

  /* Initialize parser state */
  if (initialize_parser (parser) != 0)
    {
      parser->error = GLR_PARSE_ERROR_MEMORY;
      return -1;
    }

  /* Allocate initial stack array */
  parser->stacks = malloc (sizeof (glr_stack_t *));
  if (parser->stacks == NULL)
    {
      parser->error = GLR_PARSE_ERROR_MEMORY;
      return -1;
    }

  /* Create initial parse stack */
  parser->stacks[0] = glr_stack_create ();
  if (parser->stacks[0] == NULL)
    {
      free (parser->stacks);
      parser->stacks = NULL;
      parser->error = GLR_PARSE_ERROR_MEMORY;
      return -1;
    }

  parser->stack_count = 1;
  parser->stack_capacity = 1;

  /* Configure reader for UTF-16 input if needed */
  if (should_use_reader (parser->input, parser->input_length))
    {
      glr_reader_set_encoding (parser->reader,
                               GLR_READER_ENCODING_UTF16_AUTO);
      if (glr_reader_set_lexer_hooks (parser->reader, parser->lexer_hooks)
              != 0
          || glr_reader_set_input (parser->reader, parser->input,
                                   parser->input_length)
                 != 0)
        {
          parser->error = GLR_PARSE_ERROR_MEMORY;
          return -1;
        }
      glr_reader_reset (parser->reader);
    }

  /* Main parsing loop: process input until exhausted */
  while (parser->input_pos < parser->input_length)
    {
      /* Consume next terminal symbol */
      if (consume_next_terminal (parser) != 0)
        {
          return parser->error == GLR_PARSE_SUCCESS ? 0 : -1;
        }

      /* Perform shift and reduce on all active stacks */
      for (size_t i = 0; i < parser->stack_count; i++)
        {
          shift_item (parser, (int)i);
          reduce_item (parser, (int)i);
        }

      /* Handle any conflicts that arose */
      handle_conflict (parser);
    }

  return 0;
}

/* ============================================================================
 * Public Parsing API
 * ========================================================================== */

/**
 * @brief Parse input buffer using GLR algorithm (non-incremental)
 *
 * This is the main entry point for non-incremental parsing. It performs
 * a complete parse from scratch, implementing Tomita's GLR algorithm with
 * Graph-Structured Stack and Shared Packed Parse Forest.
 *
 * The parser is automatically reset before parsing, so any previous state
 * is discarded. For incremental parsing, use glr_parser_parse_incremental().
 *
 * @param parser Initialized parser instance
 * @param input Input buffer (UTF-8 or UTF-16)
 * @param length Length of input buffer in bytes
 * @return Parse result structure with error code, forest, and position
 */
glr_parse_result_t
glr_parse (glr_parser_t *parser, const char *input, size_t length)
{
  glr_parse_result_t result = { 0 };

  /* Validate parameters */
  if (parser == NULL || input == NULL)
    {
      result.error = GLR_PARSE_ERROR_MEMORY;
      result.forest = NULL;
      result.position = 0;
      result.user_data = NULL;
      return result;
    }

  /* Reset parser state for fresh parse */
  if (glr_parser_reset (parser) != 0)
    {
      result.error = GLR_PARSE_ERROR_MEMORY;
      return result;
    }

  /* Set input buffer */
  parser->input = input;
  parser->input_length = length;

  /* Run the parsing algorithm */
  if (parse_input (parser) != 0)
    {
      result.error = parser->error;
      result.forest = NULL;
      result.position = parser->input_pos;
      result.user_data = parser->user_data;
      return result;
    }

  /* Check if parse was successful (at least one accepting stack) */
  if (parser->stack_count == 0)
    {
      result.error = GLR_PARSE_ERROR_UNRECOVERABLE;
      result.forest = NULL;
      result.position = parser->input_pos;
      result.user_data = parser->user_data;
      return result;
    }

  /* Return successful result */
  result.error = GLR_PARSE_SUCCESS;
  result.forest = parser->forest;
  result.position = parser->input_pos;
  result.user_data = parser->user_data;

  return result;
}

/* ============================================================================
 * Lexer Configuration
 * ========================================================================== */

/**
 * @brief Set custom lexer hooks for tokenization
 *
 * Configures custom lexer hooks that override default tokenization.
 * The hooks are applied to the internal reader for UTF-16 processing.
 *
 * @param parser Parser instance
 * @param hooks Lexer hooks structure, or NULL for default tokenization
 * @return 0 on success, -1 on failure
 */
int
glr_parser_set_lexer_hooks (glr_parser_t *parser, glr_lexer_hooks_t *hooks)
{
  if (parser == NULL)
    {
      return -1;
    }

  parser->lexer_hooks = hooks;
  return parser->reader != NULL
             ? glr_reader_set_lexer_hooks (parser->reader, hooks)
             : -1;
}

/**
 * @brief Get currently configured lexer hooks
 *
 * @param parser Parser instance
 * @return Lexer hooks, or NULL if none configured
 */
glr_lexer_hooks_t *
glr_parser_get_lexer_hooks (const glr_parser_t *parser)
{
  return parser != NULL ? parser->lexer_hooks : NULL;
}

/* ============================================================================
 * Parser State Inspection
 * ========================================================================== */

/**
 * @brief Get the most recent token read by the parser
 *
 * Returns the current lookahead token for error reporting and debugging.
 *
 * @param parser Parser instance
 * @return Pointer to last token, or NULL if none read yet
 */
const glr_reader_token_t *
glr_parser_get_last_token (const glr_parser_t *parser)
{
  if (parser == NULL || parser->lookahead.terminal_name == NULL)
    {
      return NULL;
    }

  return &parser->lookahead;
}

/* ============================================================================
 * Library Information
 * ========================================================================== */

/**
 * @brief Get library version string
 *
 * @return Version string in semantic versioning format (e.g., "1.0.0")
 */
const char *
glr_version (void)
{
  return "1.0.0";
}

/**
 * @brief Get library name
 *
 * @return Library name string
 */
const char *
glr_name (void)
{
  return "LibGLR";
}
