#include <glr/grammar.h>
#include <glr/parser.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int parse_input (glr_parser_t *parser);
static int shift_item (glr_parser_t *parser, int stack_idx);
static int reduce_item (glr_parser_t *parser, int stack_idx);
static void handle_conflict (glr_parser_t *parser);
static int initialize_parser (glr_parser_t *parser);
static int grammar_accepts_token (const glr_grammar_t *grammar, const char *name);
static int should_use_reader (const char *input, size_t length);
static int consume_next_terminal (glr_parser_t *parser);

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

  parser->grammar = grammar;
  parser->stacks = NULL;
  parser->stack_count = 0;
  parser->stack_capacity = 0;
  parser->forest = glr_forest_create ();
  parser->state_table = NULL;
  parser->state_table_size = 0;
  parser->disambig_hooks = NULL;
  parser->input = NULL;
  parser->input_pos = 0;
  parser->input_length = 0;
  parser->reader = glr_reader_create ();
  parser->lexer_hooks = NULL;
  memset (&parser->lookahead, 0, sizeof (parser->lookahead));
  parser->error = GLR_PARSE_SUCCESS;
  parser->user_data = NULL;

  if (parser->reader == NULL)
    {
      glr_forest_destroy (parser->forest);
      free (parser);
      return NULL;
    }

  return parser;
}

void
glr_parser_destroy (glr_parser_t *parser)
{
  if (parser == NULL)
    {
      return;
    }

  for (size_t i = 0; i < parser->stack_count; i++)
    {
      glr_stack_destroy (parser->stacks[i]);
    }
  free (parser->stacks);

  glr_forest_destroy (parser->forest);

  free (parser->state_table);
  glr_reader_token_clear (&parser->lookahead);
  glr_reader_destroy (parser->reader);

  glr_parser_clear_disambiguators (parser);

  free (parser);
}

int
glr_parser_reset (glr_parser_t *parser)
{
  if (parser == NULL)
    {
      return -1;
    }

  for (size_t i = 0; i < parser->stack_count; i++)
    {
      glr_stack_destroy (parser->stacks[i]);
    }
  free (parser->stacks);

  glr_forest_destroy (parser->forest);
  parser->forest = glr_forest_create ();

  parser->stacks = NULL;
  parser->stack_count = 0;
  parser->stack_capacity = 0;
  glr_reader_token_clear (&parser->lookahead);
  parser->input_pos = 0;
  parser->error = GLR_PARSE_SUCCESS;

  return 0;
}

static int
initialize_parser (glr_parser_t *parser)
{
  if (parser == NULL || parser->grammar == NULL)
    {
      return -1;
    }

  if (parser->state_table == NULL)
    {
      parser->state_table_size = 0;
    }

  return 0;
}

static int
grammar_accepts_token (const glr_grammar_t *grammar, const char *name)
{
  size_t i;

  if (grammar == NULL || name == NULL)
    {
      return 0;
    }

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

static int
should_use_reader (const char *input, size_t length)
{
  size_t i;

  if (input == NULL || length < 2 || (length % 2) != 0)
    {
      return 0;
    }

  if (((const unsigned char *)input)[0] == 0xFF
      && ((const unsigned char *)input)[1] == 0xFE)
    {
      return 1;
    }
  if (((const unsigned char *)input)[0] == 0xFE
      && ((const unsigned char *)input)[1] == 0xFF)
    {
      return 1;
    }

  for (i = 1; i < length; i += 2)
    {
      if (((const unsigned char *)input)[i] == 0)
        {
          return 1;
        }
    }

  return 0;
}

static int
consume_next_terminal (glr_parser_t *parser)
{
  if (should_use_reader (parser->input, parser->input_length))
    {
      glr_reader_status_t status;

      status = glr_reader_next (parser->reader, &parser->lookahead);
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

      parser->input_pos = parser->lookahead.byte_offset + parser->lookahead.bytes_consumed;
      if (!grammar_accepts_token (parser->grammar, parser->lookahead.terminal_name))
        {
          parser->error = GLR_PARSE_ERROR_SYNTAX;
          return -1;
        }
      return 0;
    }

  glr_reader_token_clear (&parser->lookahead);
  parser->input_pos++;
  return 0;
}

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

  void *current_state = glr_stack_peek (stack);
  if (current_state == NULL)
    {
      return -1;
    }

  if (parser->input_pos >= parser->input_length)
    {
      return 0;
    }

  return 0;
}

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

  return 0;
}

static void
handle_conflict (glr_parser_t *parser)
{
  if (parser == NULL)
    {
      return;
    }

  if (parser->disambig_hooks != NULL)
    {
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
      (void)glr_parser_run_disambiguators (parser, &context, NULL);
    }
}

static int
parse_input (glr_parser_t *parser)
{
  if (parser == NULL || parser->grammar == NULL)
    {
      return -1;
    }

  if (parser->grammar->start_symbol == NULL)
    {
      parser->error = GLR_PARSE_ERROR_GRAMMAR;
      return -1;
    }

  if (initialize_parser (parser) != 0)
    {
      parser->error = GLR_PARSE_ERROR_MEMORY;
      return -1;
    }

  parser->stacks = malloc (sizeof (glr_stack_t *));
  if (parser->stacks == NULL)
    {
      parser->error = GLR_PARSE_ERROR_MEMORY;
      return -1;
    }

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

  if (should_use_reader (parser->input, parser->input_length))
    {
      glr_reader_set_encoding (parser->reader, GLR_READER_ENCODING_UTF16_AUTO);
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

  while (parser->input_pos < parser->input_length)
    {
      if (consume_next_terminal (parser) != 0)
        {
          return parser->error == GLR_PARSE_SUCCESS ? 0 : -1;
        }

      for (size_t i = 0; i < parser->stack_count; i++)
        {
          shift_item (parser, (int)i);
          reduce_item (parser, (int)i);
        }

      handle_conflict (parser);
    }

  return 0;
}

glr_parse_result_t
glr_parse (glr_parser_t *parser, const char *input, size_t length)
{
  glr_parse_result_t result = { 0 };

  if (parser == NULL || input == NULL)
    {
      result.error = GLR_PARSE_ERROR_MEMORY;
      result.forest = NULL;
      result.position = 0;
      result.user_data = NULL;
      return result;
    }

  if (glr_parser_reset (parser) != 0)
    {
      result.error = GLR_PARSE_ERROR_MEMORY;
      return result;
    }

  parser->input = input;
  parser->input_length = length;

  if (parse_input (parser) != 0)
    {
      result.error = parser->error;
      result.forest = NULL;
      result.position = parser->input_pos;
      result.user_data = parser->user_data;
      return result;
    }

  if (parser->stack_count == 0)
    {
      result.error = GLR_PARSE_ERROR_UNRECOVERABLE;
      result.forest = NULL;
      result.position = parser->input_pos;
      result.user_data = parser->user_data;
      return result;
    }

  result.error = GLR_PARSE_SUCCESS;
  result.forest = parser->forest;
  result.position = parser->input_pos;
  result.user_data = parser->user_data;

  return result;
}

int
glr_parser_set_lexer_hooks (glr_parser_t *parser, glr_lexer_hooks_t *hooks)
{
  if (parser == NULL)
    {
      return -1;
    }

  parser->lexer_hooks = hooks;
  return parser->reader != NULL ? glr_reader_set_lexer_hooks (parser->reader, hooks)
                                : -1;
}

glr_lexer_hooks_t *
glr_parser_get_lexer_hooks (const glr_parser_t *parser)
{
  return parser != NULL ? parser->lexer_hooks : NULL;
}

const glr_reader_token_t *
glr_parser_get_last_token (const glr_parser_t *parser)
{
  if (parser == NULL || parser->lookahead.terminal_name == NULL)
    {
      return NULL;
    }

  return &parser->lookahead;
}

const char *
glr_version (void)
{
  return "1.0.0";
}

const char *
glr_name (void)
{
  return "LibGLR";
}
