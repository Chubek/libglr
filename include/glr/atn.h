#ifndef GLR_ATN_H
#define GLR_ATN_H

/**
 * @file atn.h
 * @brief Augmented Transition Network (ATN) support for GLR front ends.
 *
 * An ATN is a compact epsilon-NFA representation of a grammar's production
 * alternatives.  libglr uses it as a generator/runtime intermediate: a rule
 * entry has epsilon edges to the alternatives for that rule and every
 * production is represented by a linear path of symbol transitions.
 *
 * This deliberately keeps the ATN independent of the LR parse table.  A
 * front end can therefore use it for production matching, lookahead,
 * diagnostics, or code generation and still hand the same grammar to the
 * ordinary GLR parser.
 */

#include <glr/grammar.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct glr_atn glr_atn_t;
typedef struct glr_atn_state_set glr_atn_state_set_t;

typedef enum
{
  GLR_ATN_EPSILON = 0,
  GLR_ATN_SYMBOL = 1
} glr_atn_transition_type_t;

typedef struct
{
  glr_atn_transition_type_t type;
  uint32_t target;
  int symbol_id;
} glr_atn_transition_t;

typedef struct
{
  glr_atn_transition_t *transitions;
  size_t transition_count;
  size_t transition_capacity;
  bool accepting;
  int production_id;
} glr_atn_state_t;

/** Create an empty ATN with state zero as its start state. */
glr_atn_t *glr_atn_create(void);

/** Destroy an ATN. NULL is accepted. */
void glr_atn_destroy(glr_atn_t *atn);

/** Add a state and return its numeric id, or UINT32_MAX on failure. */
uint32_t glr_atn_add_state(glr_atn_t *atn);

/** Add an epsilon transition. */
int glr_atn_add_epsilon(glr_atn_t *atn, uint32_t from, uint32_t to);

/** Add a transition consuming one grammar symbol. */
int glr_atn_add_symbol(glr_atn_t *atn, uint32_t from, uint32_t to,
                       int symbol_id);

/** Mark a state as accepting for a production (production_id may be -1). */
int glr_atn_set_accepting(glr_atn_t *atn, uint32_t state, bool accepting,
                          int production_id);

/** Return the ATN's start state, or UINT32_MAX for NULL. */
uint32_t glr_atn_start_state(const glr_atn_t *atn);

/** Return the number of states. */
size_t glr_atn_state_count(const glr_atn_t *atn);

/** Return a state by id, or NULL. */
const glr_atn_state_t *glr_atn_state(const glr_atn_t *atn, uint32_t state);

/** Return a non-owning set of ATN state ids. */
glr_atn_state_set_t *glr_atn_state_set_create(void);
void glr_atn_state_set_destroy(glr_atn_state_set_t *set);
void glr_atn_state_set_clear(glr_atn_state_set_t *set);
int glr_atn_state_set_add(glr_atn_state_set_t *set, uint32_t state);
bool glr_atn_state_set_contains(const glr_atn_state_set_t *set, uint32_t state);
size_t glr_atn_state_set_count(const glr_atn_state_set_t *set);
uint32_t glr_atn_state_set_at(const glr_atn_state_set_t *set, size_t index);

/**
 * Compute epsilon closure in-place.  Repeated epsilon edges are handled and
 * duplicate states are suppressed.
 */
int glr_atn_epsilon_closure(const glr_atn_t *atn, glr_atn_state_set_t *set);

/**
 * Consume one symbol from a state set and compute the epsilon closure of the
 * resulting states.  The output may alias neither implementation storage nor
 * the input set; aliasing the two public set objects is supported.
 */
int glr_atn_step(const glr_atn_t *atn, const glr_atn_state_set_t *current,
                 int symbol_id, glr_atn_state_set_t *next);

/** True if the set contains at least one accepting state. */
bool glr_atn_state_set_accepting(const glr_atn_t *atn,
                                  const glr_atn_state_set_t *set);

/**
 * Match a complete sequence of grammar symbol ids against the ATN.
 * Returns 1 for a match, 0 for rejection, and -1 on invalid input/memory
 * failure.  This is an NFA/ATN operation; it does not perform CFG recursion.
 */
int glr_atn_match(const glr_atn_t *atn, const int *symbols, size_t count);

/** Match starting at an explicit ATN state (useful for a grammar rule entry). */
int glr_atn_match_from(const glr_atn_t *atn, uint32_t start_state,
                       const int *symbols, size_t count);

/**
 * Compile grammar productions into an ATN.  Each nonterminal gets a rule
 * entry state; that state epsilon-branches to the production paths belonging
 * to the nonterminal.  Production paths are independently accepting, which
 * makes the result useful for GLR reduction/production matching.
 */
glr_atn_t *glr_atn_from_grammar(const glr_grammar_t *grammar);

/** Return the rule entry state for a nonterminal symbol, or UINT32_MAX. */
uint32_t glr_atn_rule_start(const glr_atn_t *atn, int nonterminal_id);

#ifdef __cplusplus
}
#endif

#endif /* GLR_ATN_H */
