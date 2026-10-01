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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Forward Declarations
 * ========================================================================== */

/* Cap on candidates reported to disambiguation hooks for one cell. */
#define GLR_MAX_CELL_ACTIONS 32

/* strdup() is not part of C11, so the parser carries its own copy helper. */
static char *
parser_strdup (const char *text)
{
  size_t length;
  char *copy;

  if (text == NULL)
    {
      return NULL;
    }

  length = strlen (text);
  copy = malloc (length + 1);
  if (copy == NULL)
    {
      return NULL;
    }

  memcpy (copy, text, length + 1);
  return copy;
}

typedef struct
{
  const glr_stack_t *seed_stack; /* stack to resume from, or NULL */
  glr_forest_t *shared_forest;   /* forest to pack into, or NULL for the parser's own */
  size_t start_position;         /* input offset the seed applies to */
} glr_parse_seed_t;

static int parse_input (glr_parser_t *parser, const glr_parse_seed_t *seed,
                        bool *accepted);
static int initialize_parser (glr_parser_t *parser);
static glr_parse_table_t *get_active_parse_table (const glr_parser_t *parser);
static int grammar_find_symbol_id (const glr_grammar_t *grammar,
                                   const char *name,
                                   glr_symbol_type_t type);
static int parser_get_lookahead_symbol_id (glr_parser_t *parser);
static int parser_append_stack (glr_parser_t *parser, glr_stack_t *stack);
static int parser_prune_stack (glr_parser_t *parser, size_t stack_idx);
static int apply_reduction_action (glr_parser_t *parser, glr_stack_t *stack,
                                   const glr_action_t *action);
static int parser_push_entry (glr_stack_t *stack, uint32_t state,
                              glr_forest_node_t *node);
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
  parser->stack_skip_reduce = NULL;
  parser->stack_count = 0;
  parser->stack_capacity = 0;
  parser->forest = glr_forest_create ();
  parser->state_table = NULL;
  parser->state_table_size = 0;
  parser->parse_table = NULL;
  parser->owns_parse_table = false;
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
  parser->cache = NULL;

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
  free (parser->stack_skip_reduce);

  /* Free parse forest */
  glr_forest_destroy (parser->forest);

  /* Free reader resources and any parser-owned parse table */
  parser->state_table = NULL;
  if (parser->owns_parse_table)
    {
      glr_parse_table_destroy (parser->parse_table);
    }
  glr_reader_token_clear (&parser->lookahead);
  glr_reader_token_clear (&parser->last_token);
  glr_reader_destroy (parser->reader);
  free (parser->trivia);
  parser->trivia = NULL;

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
  free (parser->stack_skip_reduce);

  /* Reset parse forest */
  glr_forest_destroy (parser->forest);
  parser->forest = glr_forest_create ();

  /* Reset stack state */
  parser->stacks = NULL;
  parser->stack_skip_reduce = NULL;
  parser->stack_count = 0;
  parser->stack_capacity = 0;

  /* Clear lookahead and last-token state, reset position */
  glr_reader_token_clear (&parser->lookahead);
  glr_reader_token_clear (&parser->last_token);
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
  glr_parse_table_t *table;

  if (parser == NULL || parser->grammar == NULL)
    {
      return -1;
    }

  table = get_active_parse_table (parser);

  /* No table was attached by the caller: build an SLR(1) one for the
     grammar so the engine has real actions to execute. The generated
     table is cached on the parser and owned by it. */
  if (table == NULL)
    {
      char error[128];
      table = glr_grammar_build_parse_table (parser->grammar, error,
                                             sizeof (error));
      if (table == NULL)
        {
          parser->error = GLR_PARSE_ERROR_GRAMMAR;
          return -1;
        }
      parser->parse_table = table;
      parser->owns_parse_table = true;
    }

  parser->state_table = table->state_count > 0 ? (void **)table->states : NULL;
  parser->state_table_size = table->state_count;

  return 0;
}

static glr_parse_table_t *
get_active_parse_table (const glr_parser_t *parser)
{
  if (parser == NULL)
    {
      return NULL;
    }

  if (parser->parse_table != NULL)
    {
      return parser->parse_table;
    }

  return parser->grammar != NULL ? parser->grammar->parse_table : NULL;
}

static int
grammar_find_symbol_id (const glr_grammar_t *grammar, const char *name,
                        glr_symbol_type_t type)
{
  if (grammar == NULL || name == NULL)
    {
      return -1;
    }

  for (size_t i = 0; i < grammar->symbol_count; i++)
    {
      glr_symbol_t *symbol = grammar->symbols[i];
      if (symbol != NULL && symbol->type == type && symbol->name != NULL
          && strcmp (symbol->name, name) == 0)
        {
          return symbol->id;
        }
    }

  return -1;
}

/**
 * @brief Resolve the id of the terminal currently under the cursor
 *
 * For reader-driven (UTF-16) input the terminal name comes from the token
 * the reader produced. For byte input the parser runs a longest-match
 * tokenizer over the grammar's own terminal names, so multi-character
 * terminals such as "NUMBER" or "->" work without a separate lexer.
 *
 * @param parser Parser instance
 * @return Terminal id, or -1 when at end of input or no terminal matches
 */
static int
parser_get_lookahead_symbol_id (glr_parser_t *parser)
{
  const glr_grammar_t *grammar;
  size_t pos;
  size_t remaining;
  size_t best_length = 0;
  int best_id = -1;

  if (parser == NULL || parser->grammar == NULL)
    {
      return -1;
    }

  if (parser->lookahead.terminal_name != NULL)
    {
      return grammar_find_symbol_id (parser->grammar,
                                     parser->lookahead.terminal_name,
                                     GLR_SYMBOL_TERMINAL);
    }

  grammar = parser->grammar;
  if (parser->input == NULL || parser->input_pos >= parser->input_length)
    {
      return -1; /* end of input */
    }

  /* Trivia is skipped before any terminal is matched, so grammars never
     need an (ambiguous) rule for optional whitespace. */
  if (parser->trivia != NULL)
    {
      size_t trivia_length = strlen (parser->trivia);
      if (trivia_length > 0)
        {
          while (parser->input_pos + trivia_length <= parser->input_length
                 && memcmp (parser->input + parser->input_pos,
                            parser->trivia, trivia_length)
                        == 0)
            {
              parser->input_pos += trivia_length;
            }
          if (parser->input_pos >= parser->input_length)
            {
              return -1; /* only trivia left */
            }
        }
    }

  pos = parser->input_pos;
  remaining = parser->input_length - pos;

  /* Longest match wins so that a terminal named "<<" is preferred over
     two occurrences of "<". */
  for (size_t i = 0; i < grammar->symbol_count; i++)
    {
      glr_symbol_t *symbol = grammar->symbols[i];
      size_t name_length;

      if (symbol == NULL || !glr_symbol_is_terminal (symbol)
          || symbol->name == NULL)
        {
          continue;
        }
      name_length = strlen (symbol->name);
      if (name_length == 0 || name_length > remaining
          || name_length <= best_length)
        {
          continue;
        }
      if (memcmp (parser->input + pos, symbol->name, name_length) != 0)
        {
          continue;
        }
      best_length = name_length;
      best_id = symbol->id;
    }

  return best_id;
}

static int
parser_append_stack (glr_parser_t *parser, glr_stack_t *stack)
{
  if (parser == NULL || stack == NULL)
    {
      return -1;
    }

  if (parser->stack_count == parser->stack_capacity)
    {
      size_t new_capacity
          = parser->stack_capacity == 0 ? 4 : parser->stack_capacity * 2;
      glr_stack_t **new_stacks
          = realloc (parser->stacks, new_capacity * sizeof (*new_stacks));
      bool *new_flags
          = realloc (parser->stack_skip_reduce, new_capacity * sizeof (bool));

      if (new_stacks == NULL || new_flags == NULL)
        {
          free (new_stacks);
          free (new_flags);
          return -1;
        }
      parser->stacks = new_stacks;
      parser->stack_skip_reduce = new_flags;
      parser->stack_capacity = new_capacity;
    }

  parser->stacks[parser->stack_count] = stack;
  parser->stack_skip_reduce[parser->stack_count] = false;
  parser->stack_count++;
  return 0;
}

static int
parser_prune_stack (glr_parser_t *parser, size_t stack_idx)
{
  if (parser == NULL || stack_idx >= parser->stack_count)
    {
      return -1;
    }

  glr_stack_destroy (parser->stacks[stack_idx]);
  for (size_t i = stack_idx + 1; i < parser->stack_count; i++)
    {
      parser->stacks[i - 1] = parser->stacks[i];
      parser->stack_skip_reduce[i - 1] = parser->stack_skip_reduce[i];
    }
  parser->stack_count--;
  return 0;
}

/**
 * @brief Push one GSS entry pairing a state with its SPPF node
 *
 * Stack entries are heap-allocated GSS nodes so each entry can carry both
 * the automaton state and the forest node for that prefix. Entries are
 * owned by the stack: glr_stack_destroy() releases every one of them.
 */
static int
parser_push_entry (glr_stack_t *stack, uint32_t state,
                   glr_forest_node_t *node)
{
  glr_stack_node_t *entry = glr_stack_node_create (state, 0);

  if (entry == NULL)
    {
      return -1;
    }
  if (glr_stack_node_set_forest_node (entry, node) != 0)
    {
      glr_stack_node_destroy (entry);
      return -1;
    }
  if (glr_stack_push (stack, entry) != 0)
    {
      glr_stack_node_destroy (entry);
      return -1;
    }
  return 0;
}

/**
 * @brief Read the automaton state stored in a stack entry
 */
static uint32_t
parser_entry_state (glr_stack_t *stack)
{
  glr_stack_node_t *entry;

  if (stack == NULL || glr_stack_empty (stack))
    {
      return 0;
    }
  entry = (glr_stack_node_t *) glr_stack_peek (stack);
  return glr_stack_node_get_state (entry);
}

/**
 * @brief Read the state stored `depth` entries below the top of a stack
 */
static uint32_t
parser_entry_state_after (glr_stack_t *stack, size_t depth)
{
  size_t height = glr_stack_height (stack);

  if (height <= depth)
    {
      return 0;
    }
  return glr_stack_node_get_state (
      (glr_stack_node_t *) glr_stack_get (stack, height - depth - 1));
}

static int
apply_reduction_action (glr_parser_t *parser, glr_stack_t *stack,
                        const glr_action_t *action)
{
  glr_production_t *production;
  glr_forest_node_t **children = NULL;
  glr_forest_node_t *node = NULL;
  glr_parse_table_t *table;
  size_t body_length;
  size_t start_position = 0;
  size_t end_position = 0;
  uint32_t next_state = 0;
  int rc = 0;

  if (parser == NULL || stack == NULL || action == NULL
      || action->type != GLR_ACTION_REDUCE)
    {
      return -1;
    }

  production = glr_grammar_get_production (parser->grammar,
                                           (int) action->reduce.production_id);
  if (production == NULL)
    {
      return -1;
    }

  body_length = production->body_length;

  /* The bottom of every stack holds the augmented start state, so a
     production of length n requires n + 1 entries. */
  if (glr_stack_height (stack) < body_length + 1)
    {
      return -1;
    }

  /* Resolve the GOTO target *before* touching the stack: a production that
     cannot be applied must leave the stack exactly as it was, otherwise a
     dead alternative would silently truncate the branch. */
  table = get_active_parse_table (parser);
  if (table != NULL)
    {
      uint32_t below_state = parser_entry_state_after (stack, body_length);
      if (glr_parse_table_get_goto (table, below_state,
                                    (uint32_t) production->head->id,
                                    &next_state)
          != 0)
        {
          return -1;
        }
    }

  if (body_length > 0)
    {
      size_t i;

      children = calloc (body_length, sizeof (*children));
      if (children == NULL)
        {
          return -1;
        }

      /* Pop the matched body, keeping the forest node each entry packed.
         Popping removes the entry from the stack, so its reference is
         released here once the packed node has been read out. */
      for (i = body_length; i > 0; i--)
        {
          glr_stack_node_t *entry
              = (glr_stack_node_t *) glr_stack_pop (stack);
          glr_forest_node_t *child
              = glr_stack_node_get_forest_node (entry);
          children[i - 1] = child;
          /* The first pop is the last symbol of the production, so it
             determines where the reduced span ends. */
          if (child != NULL && i == body_length)
            {
              end_position = child->end_position;
            }
          glr_stack_node_release (entry);
        }

      /* The reduced span runs from the start of the first matched child to
         the end of the last. */
      if (children[0] != NULL)
        {
          start_position = children[0]->position;
        }
      if (end_position < start_position)
        {
          end_position = start_position;
        }

      /* Each reduction becomes a constructor node keyed by
         (production, start, end). That key can never collide with one of
         its own children, so a left-recursive production such as
         E -> E + T packs without ever linking a node to itself. */
      node = glr_forest_get_constructor (parser->forest, production->id,
                                         start_position, end_position);
      if (node == NULL)
        {
          rc = -1;
          goto cleanup;
        }

      /* Attach each distinct child once so repeated reductions over the
         same span share a single packed node. */
      for (i = 0; i < body_length; i++)
        {
          bool present = false;
          size_t k;

          if (children[i] == NULL)
            {
              continue;
            }
          for (k = 0; k < node->child_count; k++)
            {
              if (node->children[k] == children[i])
                {
                  present = true;
                  break;
                }
            }
          if (!present && glr_forest_add_child (node, children[i]) != 0)
            {
              rc = -1;
              goto cleanup;
            }
        }

      /* Pack the constructor under a symbol node for the head, so several
         derivations of the same non-terminal occurrence become siblings of
         one node instead of overwriting each other. */
      {
        glr_forest_node_t *symbol = glr_forest_get_symbol (
            parser->forest, production->head->id, start_position,
            end_position);
        if (symbol == NULL)
          {
            rc = -1;
            goto cleanup;
          }
        if (glr_forest_add_child (symbol, node) != 0)
            {
              rc = -1;
              goto cleanup;
            }
        node = symbol;
      }
    }
  else
    {
      /* Empty production: it spans nothing at the current position. */
      node = glr_forest_get_symbol (parser->forest, production->head->id,
                                    parser->input_pos, parser->input_pos);
      if (node == NULL)
        {
          rc = -1;
          goto cleanup;
        }
    }

  table = get_active_parse_table (parser);
  if (table == NULL)
    {
      goto cleanup;
    }

  rc = parser_push_entry (stack, next_state, node);
  cleanup:
  free (children);
  return rc;
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
 * A UTF-16 byte order mark is the unambiguous signal and is always honored.
 * For BOM-less input the parser only accepts the strict alternating-NUL
 * shape of ASCII-range UTF-16LE, i.e. an even length where *every* odd byte
 * is zero. The previous looser test (any single NUL in an odd position)
 * misclassified ordinary binary and padded ASCII input as UTF-16.
 *
 * @param input Input buffer
 * @param length Buffer length in bytes
 * @return 1 if the buffer should be read as UTF-16, 0 otherwise
 */
static int
should_use_reader (const char *input, size_t length)
{
  if (input == NULL || length < 2 || (length % 2) != 0)
    {
      return 0;
    }

  if (((const unsigned char *)input)[0] == 0xFF
      && ((const unsigned char *)input)[1] == 0xFE)
    {
      return 1; /* UTF-16 LE BOM */
    }

  if (((const unsigned char *)input)[0] == 0xFE
      && ((const unsigned char *)input)[1] == 0xFF)
    {
      return 1; /* UTF-16 BE BOM */
    }

  for (size_t i = 1; i < length; i += 2)
    {
      if (((const unsigned char *)input)[i] != 0x00)
        {
          return 0; /* not the strict ASCII-range UTF-16LE shape */
        }
    }

  return 1;
}

/**
 * @brief Advance to the next terminal token
 *
 * Reader-driven input asks the reader for the next token; byte input is
 * tokenized on demand by parser_get_lookahead_symbol_id() against the
 * grammar's terminal names, so the cursor is only moved once a shift has
 * consumed the token.
 *
 * @param parser Parser instance
 * @return 0 on success, 1 on EOF, -1 on error
 */
static int
consume_next_terminal (glr_parser_t *parser)
{
  if (!should_use_reader (parser->input, parser->input_length))
    {
      /* Byte input: the cursor is advanced by the shift phase. */
      glr_reader_token_clear (&parser->lookahead);
      return 0;
    }

  {
    glr_reader_status_t status
        = glr_reader_next (parser->reader, &parser->lookahead);

    if (status == GLR_READER_STATUS_EOF)
      {
        parser->input_pos = parser->input_length;
        return 1;
      }

    if (status != GLR_READER_STATUS_OK)
      {
        parser->error = GLR_PARSE_ERROR_SYNTAX;
        return -1;
      }

    if (!grammar_accepts_token (parser->grammar,
                                parser->lookahead.terminal_name))
      {
        /* Unknown terminal: report the failing position and stop. */
        parser->input_pos
            = parser->lookahead.byte_offset + parser->lookahead.bytes_consumed;
        parser->error = GLR_PARSE_ERROR_SYNTAX;
        return -1;
      }

    return 0;
  }
}

/* ============================================================================
 * GLR Algorithm Core Functions
 * ========================================================================== */

/* Guard against pathological grammars that reduce forever at one position. */
#define GLR_MAX_REDUCE_CHAIN 4096

/**
 * @brief Reduce the top of one stack according to the table
 *
 * Applies every reduce action in the current cell. The first action is
 * applied in place; additional actions (reduce/reduce conflicts) fork the
 * stack so all alternatives continue in parallel.
 *
 * @param parser Parser instance
 * @param stack_idx Index of the stack to reduce
 * @param terminal_id Lookahead terminal, or -1 to use the EOF column
 * @return 1 when the stack changed, 0 when no reduction applied, -1 on error
 */
static int
reduce_item (glr_parser_t *parser, int stack_idx, int terminal_id,
              int chosen)
{
  glr_stack_t *stack;
  glr_parse_table_t *table;
  const glr_action_set_t *actions;
  uint32_t current_state;
  uint32_t column;
  size_t applied = 0;

  if (parser == NULL || stack_idx < 0
      || (size_t) stack_idx >= parser->stack_count)
    {
      return -1;
    }

  stack = parser->stacks[stack_idx];
  table = get_active_parse_table (parser);
  if (stack == NULL || table == NULL)
    {
      return 0;
    }

  column = terminal_id >= 0 ? (uint32_t) terminal_id
                            : glr_parse_table_eof_column (table);
  if (column >= table->terminal_count)
    {
      return 0; /* this table was not generated by lrtable.c */
    }

  current_state = parser_entry_state (stack);
  actions = glr_parse_table_get_actions (table, current_state, column);
  if (actions == NULL || actions->action_count == 0)
    {
      return 0;
    }

  /* A cell holding both a reduction and a shift is a shift/reduce conflict.
     The shift is an alternative to reducing, so keep a copy of this stack
     that the reduce pass leaves alone and the shift pass consumes. Without
     that copy the engine would silently commit to the reduction and lose
     half of every ambiguous parse. */
  for (size_t i = 0; i < actions->action_count; i++)
    {
      if (actions->actions[i].type != GLR_ACTION_SHIFT)
        {
          continue;
        }
      if (chosen >= 0 && (int) i != chosen)
        {
          break; /* a hook already chose the reduction */
        }
      {
        glr_stack_t *shift_only
            = glr_stack_fork (stack, glr_stack_height (stack));
        if (shift_only == NULL
            || parser_append_stack (parser, shift_only) != 0)
          {
            glr_stack_destroy (shift_only);
            parser->error = GLR_PARSE_ERROR_MEMORY;
            return -1;
          }
        parser->stack_skip_reduce[parser->stack_count - 1] = true;
      }
      break;
    }

  for (size_t i = 0; i < actions->action_count; i++)
    {
      const glr_action_t *action = &actions->actions[i];
      glr_stack_t *target = stack;

      if (action->type != GLR_ACTION_REDUCE)
        {
          continue;
        }
      if (chosen >= 0 && (int) i != chosen)
        {
          continue; /* a hook selected a different action */
        }

      if (applied > 0)
        {
          target = glr_stack_fork (stack, glr_stack_height (stack));
          if (target == NULL || parser_append_stack (parser, target) != 0)
            {
              glr_stack_destroy (target);
              parser->error = GLR_PARSE_ERROR_MEMORY;
              return -1;
            }
        }

      if (apply_reduction_action (parser, target, action) != 0)
        {
          /* No usable GOTO entry: this branch is dead, which is a normal
             outcome for one alternative of a conflict, not a parse error. */
          if (target != stack)
            {
              parser_prune_stack (parser, parser->stack_count - 1);
            }
          continue;
        }

      applied++;
    }

  return applied > 0 ? 1 : 0;
}

/**
 * @brief Byte length of the token currently under the cursor
 */
static size_t
parser_token_length (const glr_parser_t *parser, int terminal_id)
{
  glr_symbol_t *symbol;

  if (parser == NULL || terminal_id < 0)
    {
      return 0;
    }
  symbol = glr_grammar_get_symbol (parser->grammar, terminal_id);
  if (symbol == NULL || symbol->name == NULL)
    {
      return 1;
    }
  return strlen (symbol->name);
}

/**
 * @brief Shift the lookahead terminal on one stack
 *
 * Applies every shift action in the current cell, forking for extra
 * alternatives, and then advances the input cursor past the token.
 *
 * @param parser Parser instance
 * @param stack_idx Index of the stack to shift
 * @param terminal_id Lookahead terminal id
 * @return 1 when a shift happened, 0 when the cell has no shift, -1 on error
 */
static int
shift_item (glr_parser_t *parser, int stack_idx, int terminal_id,
            int chosen)
{
  glr_stack_t *stack;
  glr_parse_table_t *table;
  const glr_action_set_t *actions;
  uint32_t current_state;
  size_t token_length;
  size_t applied = 0;

  if (parser == NULL || stack_idx < 0
      || (size_t) stack_idx >= parser->stack_count)
    {
      return -1;
    }

  stack = parser->stacks[stack_idx];
  table = get_active_parse_table (parser);
  if (stack == NULL || table == NULL || terminal_id < 0)
    {
      return 0;
    }

  token_length = parser_token_length (parser, terminal_id);

  current_state = parser_entry_state (stack);
  actions = glr_parse_table_get_actions (table, current_state,
                                         (uint32_t) terminal_id);
  if (actions == NULL || actions->action_count == 0)
    {
      return 0;
    }

  for (size_t i = 0; i < actions->action_count; i++)
    {
      const glr_action_t *action = &actions->actions[i];
      glr_stack_t *target = stack;

      if (action->type != GLR_ACTION_SHIFT)
        {
          continue;
        }
      if (chosen >= 0 && (int) i != chosen)
        {
          continue; /* a hook selected a different action */
        }

      if (applied > 0)
        {
          target = glr_stack_fork (stack, glr_stack_height (stack));
          if (target == NULL || parser_append_stack (parser, target) != 0)
            {
              glr_stack_destroy (target);
              parser->error = GLR_PARSE_ERROR_MEMORY;
              return -1;
            }
        }

      /* Pack the terminal into the SPPF, spanning the bytes the token
         covers, then push the new state paired with that node. */
      {
        glr_forest_node_t *terminal_node
            = glr_forest_get_node (parser->forest, GLR_NODE_TERMINAL,
                                   terminal_id, parser->input_pos);
        if (terminal_node == NULL)
          {
            parser->error = GLR_PARSE_ERROR_MEMORY;
            return -1;
          }
        terminal_node->end_position = parser->input_pos + token_length;

        if (parser_push_entry (target, action->shift.next_state,
                               terminal_node)
            != 0)
          {
            parser->error = GLR_PARSE_ERROR_MEMORY;
            return -1;
          }
      }
      applied++;
    }

  return applied > 0 ? 1 : 0;
}

/**
 * @brief Merge structurally equivalent stacks
 *
 * Stacks with the same height and the same top state describe the same
 * parser configuration, so all but one is dropped. This is the graph
 * structured stack merge step; without it every conflict would multiply
 * live stacks exponentially.
 *
 * @param parser Parser instance
 */
static void
merge_equivalent_stacks (glr_parser_t *parser)
{
  size_t i;
  size_t j;

  if (parser == NULL)
    {
      return;
    }

  for (i = 0; i < parser->stack_count; i++)
    {
      for (j = i + 1; j < parser->stack_count;)
        {
          glr_stack_t *a = parser->stacks[i];
          glr_stack_t *b = parser->stacks[j];

          if (a != NULL && b != NULL
              && glr_stack_height (a) == glr_stack_height (b)
              && parser_entry_state (a) == parser_entry_state (b))
            {
              parser_prune_stack (parser, j);
              continue;
            }
          j++;
        }
    }
}

/**
 * @brief Ask registered disambiguators which action should win this cell
 *
 * Builds a context describing the current position from the actions in
 * the cell, lets the hook chain reject candidates, and reports the index of
 * the surviving action. Cells with a single action, or no registered hooks,
 * leave the choice to the parser.
 *
 * @param parser Parser instance
 * @param terminal_id Lookahead terminal id, or -1 at end of input
 * @return Index of the winning action, or -1 to keep every action
 */
static int
disambiguate_cell (glr_parser_t *parser, int terminal_id)
{
  glr_disambig_context_t context;
  glr_disambig_candidate_t candidates[GLR_MAX_CELL_ACTIONS];
  glr_parse_table_t *table;
  const glr_action_set_t *actions;
  size_t count = 0;
  size_t winner = 0;
  uint32_t current_state;
  uint32_t column;
  size_t i;

  if (parser == NULL || parser->disambig_hooks == NULL
      || parser->stack_count == 0)
    {
      return -1;
    }

  table = get_active_parse_table (parser);
  if (table == NULL || parser->stacks[0] == NULL)
    {
      return -1;
    }

  column = terminal_id >= 0 ? (uint32_t) terminal_id
                            : glr_parse_table_eof_column (table);
  current_state = parser_entry_state (parser->stacks[0]);
  actions = glr_parse_table_get_actions (table, current_state, column);
  if (actions == NULL || actions->action_count < 2)
    {
      return -1; /* no conflict to resolve */
    }

  for (i = 0; i < actions->action_count && i < GLR_MAX_CELL_ACTIONS; i++)
    {
      const glr_action_t *action = &actions->actions[i];
      glr_disambig_candidate_t *candidate = &candidates[count++];

      memset (candidate, 0, sizeof (*candidate));
      candidate->stack = parser->stacks[0];
      candidate->start_position = parser->input_pos;
      candidate->end_position = parser->input_pos;
      candidate->production
          = action->type == GLR_ACTION_REDUCE
                ? glr_grammar_get_production (
                      parser->grammar, (int) action->reduce.production_id)
                : NULL;
      candidate->node = glr_forest_get_node (
          parser->forest, GLR_NODE_NONTERMINAL,
          candidate->production != NULL ? candidate->production->head->id : -1,
          parser->input_pos);
      candidate->precedence = 0;
      candidate->associativity = GLR_DISAMBIG_ASSOC_NONE;
      candidate->score = 0.0;
      candidate->probability = 1.0;
    }

  memset (&context, 0, sizeof (context));
  context.parser = parser;
  context.grammar = parser->grammar;
  context.forest = parser->forest;
  context.candidates = candidates;
  context.candidate_count = count;
  context.lookahead_symbol_id = terminal_id;
  context.start_position = parser->input_pos;
  context.end_position = parser->input_pos;
  context.user_data = parser->user_data;

  if (glr_parser_run_disambiguators (parser, &context, &winner)
      != GLR_DISAMBIG_RESOLVED)
    {
      return -1;
    }

  if (winner >= count || glr_disambig_context_active_count (&context) != 1)
    {
      return -1;
    }

  return (int) winner;
}

/**
 * @brief Run the main parsing loop
 *
 * Implements the GLR algorithm on top of the parse table:
 * 1. Seed a single stack, either with the initial state or with a saved GSS
 *    entry when resuming
 * 2. For every lookahead token: reduce to fixpoint, then shift
 * 3. At end of input: reduce to fixpoint and look for the accept action
 * 4. Merge equivalent stacks after every phase
 *
 * @param parser Parser instance with input already set
 * @param seed Resume state, or NULL for a fresh parse
 * @param accepted Output flag set when an accept action was reached
 * @return 0 on success, -1 on error
 */
static int
parse_input (glr_parser_t *parser, const glr_parse_seed_t *seed,
             bool *accepted)
{
  bool use_reader;
  const glr_stack_t *resume = (seed != NULL) ? seed->seed_stack : NULL;
  size_t start_position = (seed != NULL) ? seed->start_position : 0;

  if (parser == NULL)
    {
      return -1;
    }

  if (initialize_parser (parser) != 0)
    {
      return -1;
    }

  parser->stacks = NULL;
  parser->stack_skip_reduce = NULL;
  parser->stack_count = 0;
  parser->stack_capacity = 0;
  parser->input_pos = start_position;

  if (parser_append_stack (parser, glr_stack_create ()) != 0)
    {
      parser->error = GLR_PARSE_ERROR_MEMORY;
      return -1;
    }
  glr_stack_set_gss_entries (parser->stacks[0], true);

  if (resume != NULL)
    {
      /* Continue with a copy of the snapshot: the caller's copy stays valid
         for a later resume, and the entries keep pointing at the shared
         packed nodes the snapshot was taken with. */
      for (size_t i = 0; i < glr_stack_snapshot_height (resume); i++)
        {
          glr_stack_node_t *entry_copy = glr_stack_node_copy (
              (const glr_stack_node_t *) glr_stack_snapshot_get (resume, i));

          if (entry_copy == NULL)
            {
              parser->error = GLR_PARSE_ERROR_MEMORY;
              return -1;
            }
          if (glr_stack_push (parser->stacks[0], entry_copy) != 0)
            {
              glr_stack_node_release (entry_copy);
              parser->error = GLR_PARSE_ERROR_MEMORY;
              return -1;
            }
        }
    }
  else if (parser_push_entry (parser->stacks[0], 0, NULL) != 0)
    {
      parser->error = GLR_PARSE_ERROR_MEMORY;
      return -1;
    }

  use_reader = should_use_reader (parser->input, parser->input_length);
  if (use_reader)
    {
      glr_reader_set_encoding (parser->reader,
                               GLR_READER_ENCODING_UTF16_AUTO);
      if (glr_reader_set_lexer_hooks (parser->reader, parser->lexer_hooks) != 0
          || glr_reader_set_input (parser->reader, parser->input,
                                   parser->input_length)
                 != 0)
        {
          parser->error = GLR_PARSE_ERROR_MEMORY;
          return -1;
        }
      glr_reader_reset (parser->reader);
    }

  if (accepted != NULL)
    {
      *accepted = false;
    }

  for (;;)
    {
      int terminal_id;
      int chosen;
      bool at_eof;
      size_t consumed = 0;

      /* --- obtain the lookahead ------------------------------------- */
      if (consume_next_terminal (parser) < 0)
        {
          return -1;
        }

      terminal_id = parser_get_lookahead_symbol_id (parser);
      at_eof = terminal_id < 0
               && (use_reader || parser->input_pos >= parser->input_length);

      if (terminal_id < 0 && !at_eof)
        {
          /* Bytes remain but no terminal matches: syntax error at the
             cursor, which is the most useful position to report. */
          parser->error = GLR_PARSE_ERROR_SYNTAX;
          return -1;
        }

      /* Let hooks pick a single action when the cell conflicts; -1 means
         the parser explores every action. */
      chosen = disambiguate_cell (parser, terminal_id);

      /* --- reduce to fixpoint ---------------------------------------- */
      for (size_t step = 0; step < GLR_MAX_REDUCE_CHAIN; step++)
        {
          bool changed = false;
          for (size_t i = 0; i < parser->stack_count;)
            {
              int rc;

              if (parser->stack_skip_reduce[i])
                {
                  i++; /* shift-only branch of a shift/reduce conflict */
                  continue;
                }
              rc = reduce_item (parser, (int) i, terminal_id, chosen);
              if (rc < 0)
                {
                  return -1;
                }
              if (rc > 0)
                {
                  changed = true;
                  continue; /* new stack appended: process it too */
                }
              i++;
            }
          merge_equivalent_stacks (parser);
          if (!changed)
            {
              break;
            }
          if (step + 1 == GLR_MAX_REDUCE_CHAIN)
            {
              parser->error = GLR_PARSE_ERROR_GRAMMAR;
              return -1;
            }
        }

      if (parser->stack_count == 0)
        {
          parser->error = GLR_PARSE_ERROR_SYNTAX;
          return -1;
        }

      /* --- accept / end of input ------------------------------------ */
      if (at_eof)
        {
          glr_parse_table_t *table = get_active_parse_table (parser);

                    for (size_t i = 0; i < parser->stack_count; i++)
            {
              const glr_action_set_t *actions;
              uint32_t state = parser_entry_state (parser->stacks[i]);
              glr_forest_node_t *root
                  = glr_stack_node_get_forest_node (
                      (glr_stack_node_t *) glr_stack_peek (parser->stacks[i]));

              actions = glr_parse_table_get_actions (
                  table, state, glr_parse_table_eof_column (table));
              if (actions == NULL)
                {
                  continue;
                }
              for (size_t a = 0; a < actions->action_count; a++)
                {
                  if (actions->actions[a].type != GLR_ACTION_ACCEPT)
                    {
                      continue;
                    }
                  if (accepted != NULL)
                    {
                      *accepted = true;
                    }
                  /* The top of an accepting stack is the start-symbol node,
                     which already has every accepted derivation packed
                     underneath it, so adopting it exposes all of them. */
                  if (parser->forest != NULL && parser->forest->root == NULL
                      && root != NULL)
                    {
                      parser->forest->root = root;
                    }
                }
            }
          return 0;
        }

      /* --- shift ----------------------------------------------------- */
      for (size_t i = 0; i < parser->stack_count;)
        {
          int rc = shift_item (parser, (int) i, terminal_id, chosen);
          if (rc < 0)
            {
              return -1;
            }
          if (rc == 0)
            {
              /* Dead branch: nothing to do with this state. */
                            parser_prune_stack (parser, i);
              if (parser->stack_count == 0)
                {
                  parser->error = GLR_PARSE_ERROR_SYNTAX;
                  return -1;
                }
              continue;
            }
          consumed++;
          i++;
        }

      if (consumed == 0)
        {
          parser->error = GLR_PARSE_ERROR_SYNTAX;
          return -1;
        }

      /* --- advance the cursor ---------------------------------------- */
      if (use_reader)
        {
          /* Remember the token we just shifted: the lookahead slot is
             cleared before the next read, but callers still want the most
             recent terminal after the parse finishes. */
          glr_reader_token_clear (&parser->last_token);
          parser->last_token = parser->lookahead;
          parser->last_token.terminal_name
              = parser_strdup (parser->lookahead.terminal_name);
          parser->input_pos
              = parser->lookahead.byte_offset
                + parser->lookahead.bytes_consumed;
        }
      else
        {
          glr_symbol_t *symbol
              = glr_grammar_get_symbol (parser->grammar, terminal_id);
          parser->input_pos
              += symbol != NULL && symbol->name != NULL ? strlen (symbol->name)
                                                       : 1;
        }

      merge_equivalent_stacks (parser);

      /* Hand out a snapshot per surviving stack so a caller can resume from
         the furthest position a later edit leaves untouched. */
      if (parser->snapshot_hook != NULL)
        {
          for (size_t i = 0; i < parser->stack_count; i++)
            {
              parser->snapshot_hook (parser->stacks[i], parser->input_pos,
                                     parser->snapshot_data);
            }
        }
    }

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
  {
    bool accepted = false;
    if (parse_input (parser, NULL, &accepted) != 0)
      {
        result.error = parser->error;
        result.forest = NULL;
        result.position = parser->input_pos;
        result.user_data = parser->user_data;
        return result;
      }

    if (!accepted)
      {
        result.error = parser->stack_count == 0
                           ? GLR_PARSE_ERROR_UNRECOVERABLE
                           : GLR_PARSE_ERROR_SYNTAX;
        result.forest = NULL;
        result.position = parser->input_pos;
        result.user_data = parser->user_data;
        return result;
      }
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

int
glr_parser_set_parse_table (glr_parser_t *parser,
                            glr_parse_table_t *parse_table,
                            bool take_ownership)
{
  if (parser == NULL)
    {
      return -1;
    }

  if (parser->owns_parse_table && parser->parse_table != NULL
      && parser->parse_table != parse_table)
    {
      glr_parse_table_destroy (parser->parse_table);
    }

  parser->parse_table = parse_table;
  parser->owns_parse_table = parse_table != NULL && take_ownership;

  return 0;
}

glr_parse_table_t *
glr_parser_get_parse_table (const glr_parser_t *parser)
{
  return parser != NULL ? parser->parse_table : NULL;
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
int
glr_parser_set_trivia (glr_parser_t *parser, const char *trivia)
{
  char *copy = NULL;

  if (parser == NULL || (trivia != NULL && trivia[0] == '\0'))
    {
      return -1;
    }

  if (trivia != NULL)
    {
      copy = parser_strdup (trivia);
      if (copy == NULL)
        {
          return -1;
        }
    }

  free (parser->trivia);
  parser->trivia = copy;
  return 0;
}

const char *
glr_parser_get_trivia (const glr_parser_t *parser)
{
  return parser != NULL ? parser->trivia : NULL;
}

glr_forest_t *
glr_parser_take_forest (glr_parser_t *parser)
{
  glr_forest_t *forest;

  if (parser == NULL)
    {
      return NULL;
    }
  forest = parser->forest;
  if (forest == NULL)
    {
      forest = glr_forest_create ();
      parser->forest = forest;
    }
  parser->forest = glr_forest_create ();
  return forest;
}

int
glr_parser_set_snapshot_hook (glr_parser_t *parser,
                              glr_parser_snapshot_fn snapshot,
                              void *user_data)
{
  if (parser == NULL)
    {
      return -1;
    }
  parser->snapshot_hook = snapshot;
  parser->snapshot_data = user_data;
  return 0;
}

int
glr_parser_parse_from (glr_parser_t *parser, const glr_stack_t *stack,
                       glr_forest_t *forest, size_t position,
                       const char *input, size_t length,
                       glr_parse_result_t *out_result)
{
  glr_parse_seed_t seed;
  glr_parse_result_t result;
  glr_forest_t *owned_forest;
  bool accepted = false;
  int rc;

  memset (&result, 0, sizeof (result));
  result.user_data = parser != NULL ? parser->user_data : NULL;

  if (parser == NULL || stack == NULL || forest == NULL || input == NULL)
    {
      result.error = GLR_PARSE_ERROR_MEMORY;
      if (out_result != NULL)
        {
          *out_result = result;
        }
      return -1;
    }
  if (position > length)
    {
      result.error = GLR_PARSE_ERROR_SYNTAX;
      if (out_result != NULL)
        {
          *out_result = result;
        }
      return -1;
    }

  /* Snapshots are recorded by the byte-oriented driver. Resuming a UTF-16
     buffer would need the reader positioned mid-stream, which is a different
     mechanism, so say so instead of silently parsing from the top. */
  if (should_use_reader (input, length))
    {
      result.error = GLR_PARSE_ERROR_GRAMMAR;
      if (out_result != NULL)
        {
          *out_result = result;
        }
      return -1;
    }

  /* Reset first: that releases the forest the snapshots were taken against,
     which is the forest the caller just took over, not one of its own. */
  if (glr_parser_reset (parser) != 0)
    {
      result.error = GLR_PARSE_ERROR_MEMORY;
      if (out_result != NULL)
        {
          *out_result = result;
        }
      return -1;
    }

  /* Pack into the caller's forest for the duration of the parse, and always
     hand the parser's own forest back afterwards. */
  owned_forest = parser->forest;
  parser->forest = forest;
  parser->input = input;
  parser->input_length = length;

  seed.seed_stack = stack;
  seed.shared_forest = forest;
  seed.start_position = position;

  rc = parse_input (parser, &seed, &accepted);
  parser->forest = owned_forest;
  result.position = parser->input_pos;
  if (rc != 0)
    {
      result.error = parser->error != GLR_PARSE_SUCCESS ? parser->error
                                                        : GLR_PARSE_ERROR_SYNTAX;
      result.forest = NULL;
    }
  else if (!accepted)
    {
      result.error = parser->stack_count == 0 ? GLR_PARSE_ERROR_UNRECOVERABLE
                                             : GLR_PARSE_ERROR_SYNTAX;
      result.forest = NULL;
    }
  else
    {
      result.error = GLR_PARSE_SUCCESS;
      result.forest = forest;
    }

  if (out_result != NULL)
    {
      *out_result = result;
    }
  return result.error == GLR_PARSE_SUCCESS ? 0 : -1;
}

const glr_reader_token_t *
glr_parser_get_last_token (const glr_parser_t *parser)
{
  /* The lookahead slot is cleared once its token has been consumed, so the
     most recent token is reported from a dedicated slot. */
  if (parser == NULL || parser->last_token.terminal_name == NULL)
    {
      return NULL;
    }

  return &parser->last_token;
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
