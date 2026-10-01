/**
 * @file calc.c
 * @brief Calculator example using LibGLR
 *
 * Demonstrates a complete GLR pipeline:
 *
 *  1. build a grammar whose terminals are the literal text of each token
 *  2. let the library construct an SLR(1) parse table
 *  3. parse an expression into a Shared Packed Parse Forest (SPPF)
 *  4. walk the forest and evaluate the accepted derivation
 *
 * Operator precedence is encoded in the grammar shape (Expr -> Expr + Term,
 * Term -> Term * Factor), so the generated table has zero conflicts and the
 * parser never has to guess.
 *
 * The forest exposes constructor nodes: one per reduction, keyed by
 * production, whose children are the matched symbols. Terminals are leaves
 * that carry the source span they cover, which is how numeric literals get
 * their value back.
 *
 * Usage: ./calc <expression>
 * Example: ./calc "1 + 2 * 3"   -> 7
 */

#include <ctype.h>
#include <glr/glr.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Build the calculator grammar
 *
 * Multi-digit numbers are handled by giving the parser one terminal per
 * digit; NUMBER is a non-terminal over those digits, so the tokenizer only
 * ever has to match a single character.
 */
static glr_grammar_t *
build_grammar (int *out_number, int *out_space)
{
  glr_grammar_t *grammar = glr_grammar_create ();
  static const char digits[] = "0123456789";
  int expr;
  int term;
  int factor;
  int number;
  int space;
  int digit[10];
  glr_symbol_t *body[3];
  size_t i;

  if (grammar == NULL)
    {
      return NULL;
    }

  expr = glr_grammar_add_symbol (grammar, GLR_SYMBOL_NONTERMINAL, "Expr");
  term = glr_grammar_add_symbol (grammar, GLR_SYMBOL_NONTERMINAL, "Term");
  factor = glr_grammar_add_symbol (grammar, GLR_SYMBOL_NONTERMINAL, "Factor");
  number = glr_grammar_add_symbol (grammar, GLR_SYMBOL_NONTERMINAL, "NUMBER");
  space = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, " ");

  for (i = 0; digits[i] != '\0'; i++)
    {
      char name[2] = { digits[i], '\0' };
      digit[i] = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, name);
    }

  {
    int plus = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, "+");
    int minus = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, "-");
    int star = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, "*");
    int slash = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, "/");
    int lparen = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, "(");
    int rparen = glr_grammar_add_symbol (grammar, GLR_SYMBOL_TERMINAL, ")");

    /* expression -> term */
    body[0] = glr_grammar_get_symbol (grammar, term);
    glr_grammar_add_production (grammar, expr, body, 1);

    /* expression -> expression + term */
    body[0] = glr_grammar_get_symbol (grammar, expr);
    body[1] = glr_grammar_get_symbol (grammar, plus);
    body[2] = glr_grammar_get_symbol (grammar, term);
    glr_grammar_add_production (grammar, expr, body, 3);

    /* expression -> expression - term */
    body[0] = glr_grammar_get_symbol (grammar, expr);
    body[1] = glr_grammar_get_symbol (grammar, minus);
    body[2] = glr_grammar_get_symbol (grammar, term);
    glr_grammar_add_production (grammar, expr, body, 3);

    /* term -> factor */
    body[0] = glr_grammar_get_symbol (grammar, factor);
    glr_grammar_add_production (grammar, term, body, 1);

    /* term -> term * factor */
    body[0] = glr_grammar_get_symbol (grammar, term);
    body[1] = glr_grammar_get_symbol (grammar, star);
    body[2] = glr_grammar_get_symbol (grammar, factor);
    glr_grammar_add_production (grammar, term, body, 3);

    /* term -> term / factor */
    body[0] = glr_grammar_get_symbol (grammar, term);
    body[1] = glr_grammar_get_symbol (grammar, slash);
    body[2] = glr_grammar_get_symbol (grammar, factor);
    glr_grammar_add_production (grammar, term, body, 3);

    /* factor -> NUMBER */
    body[0] = glr_grammar_get_symbol (grammar, number);
    glr_grammar_add_production (grammar, factor, body, 1);

    /* factor -> ( expression ) */
    body[0] = glr_grammar_get_symbol (grammar, lparen);
    body[1] = glr_grammar_get_symbol (grammar, expr);
    body[2] = glr_grammar_get_symbol (grammar, rparen);
    glr_grammar_add_production (grammar, factor, body, 3);

  }

  /* NUMBER -> digit { digit } */
  for (i = 0; i < 10; i++)
    {
      body[0] = glr_grammar_get_symbol (grammar, digit[i]);
      glr_grammar_add_production (grammar, number, body, 1);
    }
  body[0] = glr_grammar_get_symbol (grammar, number);
  body[1] = glr_grammar_get_symbol (grammar, digit[0]);
  glr_grammar_add_production (grammar, number, body, 2);
  for (i = 1; i < 10; i++)
    {
      body[0] = glr_grammar_get_symbol (grammar, number);
      body[1] = glr_grammar_get_symbol (grammar, digit[i]);
      glr_grammar_add_production (grammar, number, body, 2);
    }

  glr_grammar_set_start_symbol (grammar, expr);
  if (out_number != NULL)
    {
      *out_number = number;
    }
  if (out_space != NULL)
    {
      *out_space = 0;
    }
  return grammar;
}

/**
 * @brief Read the literal text a terminal node covers
 */
static const char *
terminal_text (const glr_forest_node_t *node, const char *input)
{
  if (node->end_position <= node->position
      || node->end_position > strlen (input))
    {
      return "";
    }
  return input + node->position;
}

/**
 * @brief Evaluate one SPPF node
 *
 * A constructor node stores its production id, so the shape of the rule is
 * read straight from the grammar: three-symbol rules are binary operators,
 * two-symbol rules either append a digit to a NUMBER or skip a space, and
 * one-symbol rules just forward their child.
 */
static bool
eval_node (glr_grammar_t *grammar, glr_forest_node_t *node, const char *input,
           double *out)
{
  glr_production_t *production;
  glr_symbol_t *left;
  glr_symbol_t *middle;
  glr_symbol_t *right;
  double a = 0.0;
  double b = 0.0;

  if (node == NULL || out == NULL)
    {
      return false;
    }

  if (node->type == GLR_NODE_TERMINAL)
    {
      *out = 0.0;
      return true;
    }

  /* A symbol node packs one constructor per accepted derivation of that
     non-terminal occurrence; the calculator's grammars are unambiguous, so
     the first constructor is the derivation. */
  if (node->type == GLR_NODE_NONTERMINAL)
    {
      if (node->child_count == 0)
        {
          *out = 0.0;
          return true;
        }
      return eval_node (grammar, node->children[0], input, out);
    }

  if (node->child_count == 0)
    {
      *out = 0.0;
      return true;
    }

  production = glr_grammar_get_production (grammar, node->symbol_id);
  if (production == NULL || production->body_length != node->child_count)
    {
      fprintf (stderr, "malformed forest node (symbol_id %d)\n",
               node->symbol_id);
      return false;
    }

  if (production->body_length == 1)
    {
      glr_forest_node_t *child = node->children[0];

      if (child->type == GLR_NODE_TERMINAL)
        {
          /* NUMBER -> digit */
          *out = terminal_text (child, input)[0] - '0';
          return true;
        }
      /* expression -> term, term -> factor, factor -> NUMBER */
      return eval_node (grammar, child, input, out);
    }

  if (production->body_length == 2)
    {
      left = production->body[0];
      middle = production->body[1];

      if (!eval_node (grammar, node->children[0], input, &a))
        {
          return false;
        }

      if (!glr_symbol_is_terminal (left)
          && isdigit ((unsigned char) middle->name[0]))
        {
          /* NUMBER -> NUMBER digit */
          *out = a * 10.0
                 + (terminal_text (node->children[1], input)[0] - '0');
          return true;
        }

      fprintf (stderr, "unsupported two-symbol rule\n");
      return false;
    }

  /* factor -> ( expression ) simply forwards the grouped value. */
  left = production->body[0];
  if (glr_symbol_is_terminal (left) && left->name[0] == '(')
    {
      return eval_node (grammar, node->children[1], input, out);
    }

  /* Binary operator: left op right */
  middle = production->body[1];

  if (!eval_node (grammar, node->children[0], input, &a)
      || !eval_node (grammar, node->children[2], input, &b))
    {
      return false;
    }

  switch (middle->name[0])
    {
    case '+':
      *out = a + b;
      return true;
    case '-':
      *out = a - b;
      return true;
    case '*':
      *out = a * b;
      return true;
    case '/':
      if (b == 0.0)
        {
          fprintf (stderr, "division by zero\n");
          return false;
        }
      *out = a / b;
      return true;
    default:
      fprintf (stderr, "unexpected operator '%c'\n", middle->name[0]);
      return false;
    }
}

int
main (int argc, char *argv[])
{
  glr_grammar_t *grammar;
  glr_parser_t *parser;
  glr_parse_result_t result;
  glr_parse_table_t *table;
  char error[128];
  int number_id = -1;
  int space_id = -1;
  double value = 0.0;

  if (argc < 2)
    {
      printf ("Usage: %s <expression>\n", argv[0]);
      printf ("Example: %s \"1 + 2 * 3\"\n", argv[0]);
      return 1;
    }

  grammar = build_grammar (&number_id, &space_id);
  if (grammar == NULL)
    {
      fprintf (stderr, "failed to build grammar\n");
      return 1;
    }

  memset (error, 0, sizeof (error));
  table = glr_grammar_build_parse_table (grammar, error, sizeof (error));
  if (table == NULL)
    {
      fprintf (stderr, "failed to build parse table: %s\n", error);
      glr_grammar_destroy (grammar);
      return 1;
    }

  printf ("Parse table: %zu states, %zu conflicts\n", table->state_count,
          glr_parse_table_conflict_count (table));

  parser = glr_parser_create (grammar);
  if (parser == NULL)
    {
      fprintf (stderr, "failed to create parser\n");
      glr_parse_table_destroy (table);
      glr_grammar_destroy (grammar);
      return 1;
    }
  glr_parser_set_parse_table (parser, table, false);
  /* Whitespace is trivia: skipping it in the tokenizer keeps the grammar
     free of ambiguous spacing rules. */
  glr_parser_set_trivia (parser, " ");

  printf ("Parsing: %s\n", argv[1]);
  result = glr_parse (parser, argv[1], strlen (argv[1]));

  if (result.error != GLR_PARSE_SUCCESS)
    {
      printf ("Parse failed: error %d at byte %zu\n", result.error,
              result.position);
      glr_parser_destroy (parser);
      glr_parse_table_destroy (table);
      glr_grammar_destroy (grammar);
      return 1;
    }

  printf ("Parse succeeded: consumed %zu bytes, forest holds %zu packed "
          "nodes, ambiguous: %s\n",
          result.position,
          result.forest != NULL ? glr_forest_total_nodes (result.forest) : 0,
          result.forest != NULL
                  && glr_forest_is_ambiguous (result.forest->root)
              ? "yes"
              : "no");

  if (result.forest == NULL || result.forest->root == NULL)
    {
      fprintf (stderr, "parse produced no root node\n");
      glr_parser_destroy (parser);
      glr_parse_table_destroy (table);
      glr_grammar_destroy (grammar);
      return 1;
    }

  if (!eval_node (grammar, result.forest->root, argv[1], &value))
    {
      glr_parser_destroy (parser);
      glr_parse_table_destroy (table);
      glr_grammar_destroy (grammar);
      return 1;
    }

  printf ("Result: %g\n", value);

  glr_parser_destroy (parser);
  glr_parse_table_destroy (table);
  glr_grammar_destroy (grammar);
  return 0;
}
