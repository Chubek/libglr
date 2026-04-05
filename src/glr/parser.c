#include <glr/grammar.h>
#include <glr/parser.h>
#include <stdlib.h>
#include <string.h>

static int parse_input (glr_parser_t *parser);
static int shift_item (glr_parser_t *parser, int stack_idx);
static int reduce_item (glr_parser_t *parser, int stack_idx);
static void handle_conflict (glr_parser_t *parser);
static int initialize_parser (glr_parser_t *parser);

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
  parser->input = NULL;
  parser->input_pos = 0;
  parser->input_length = 0;
  parser->error = GLR_PARSE_SUCCESS;
  parser->user_data = NULL;

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

  while (parser->input_pos < parser->input_length)
    {
      for (size_t i = 0; i < parser->stack_count; i++)
        {
          shift_item (parser, (int)i);
          reduce_item (parser, (int)i);
        }

      handle_conflict (parser);

      parser->input_pos++;
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
