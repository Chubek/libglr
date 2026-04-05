#ifndef GLR_DISAMBIGUATE_H
#define GLR_DISAMBIGUATE_H

/**
 * @file disambiguate.h
 * @brief GLR Disambiguation Strategies and Utilities
 * 
 * This module provides a comprehensive framework for resolving ambiguity
 * during GLR parsing. Unlike traditional vtable-based designs, this
 * architecture uses composable strategy objects that integrate naturally
 * with the rest of libglr and are friendly to embedding in extension
 * languages like Lua.
 * 
 * @section design_principles Design Principles
 * - **Composability**: Strategies can be combined using strategy composition
 * - **Embeddability**: Clean C API suitable for Lua, Python, etc.
 * - **Dependency-aware**: Leverages existing libglr interfaces
 * - **Type-safe**: Strong typing with minimal casting
 * 
 * @section integration Integration with libglr
 * This header depends on and integrates with:
 * - @ref glr_parser_t from parser.h
 * - @ref glr_forest_t from forest.h
 * - @ref glr_reduction_t from reduction.h
 * - @ref glr_grammar_t from grammar.h
 * - @ref glr_stack_t from stack.h
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

/* ===== Forward Declarations from other libglr headers ===== */

typedef struct glr_parser glr_parser_t;
typedef struct glr_forest_node glr_forest_node_t;
typedef struct glr_forest_edge glr_forest_edge_t;
typedef struct glr_reduction glr_reduction_t;
typedef struct glr_grammar glr_grammar_t;
typedef struct glr_stack glr_stack_t;
typedef struct glr_symbol glr_symbol_t;
typedef struct glr_production glr_production_t;

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Core Type Definitions ===== */

/**
 * @typedef glr_disambiguation_context_t
 * @brief Context passed during disambiguation
 * 
 * Provides access to parser state, forest structure, and reduction candidates.
 * This is the primary interface for disambiguation strategies.
 */
typedef struct glr_disambiguation_context {
    glr_parser_t *parser;           ///< Parser instance
    glr_forest_node_t *parent;      ///< Parent SPPF node
    glr_reduction_t **candidates;   ///< Candidate reductions
    size_t candidate_count;         ///< Number of candidates
    size_t input_position;          ///< Current input position
    const glr_grammar_t *grammar;   ///< Grammar being parsed
    void *user_data;                ///< User-provided data for callbacks
} glr_disambiguation_context_t;

/**
 * @typedef glr_disambiguation_result_t
 * @brief Result of disambiguation
 * 
 * Contains the list of accepted reductions after disambiguation.
 * The result array is allocated by the strategy and must be freed
 * using glr_disambiguation_result_free().
 */
typedef struct glr_disambiguation_result {
    glr_reduction_t **accepted;     ///< Accepted reductions (malloc'd array)
    size_t accepted_count;          ///< Number of accepted reductions
    void *extra_data;               ///< Strategy-specific extra data
} glr_disambiguation_result_t;

/**
 * @typedef glr_priority_t
 * @brief Priority/weight type for disambiguation
 * 
 * Higher values indicate higher priority. Special values:
 * - GLR_PRIORITY_UNSPECIFIED: No priority specified
 * - GLR_PRIORITY_MINIMUM: Lowest possible priority
 * - GLR_PRIORITY_MAXIMUM: Highest possible priority
 */
typedef int32_t glr_priority_t;

#define GLR_PRIORITY_UNSPECIFIED INT32_MIN
#define GLR_PRIORITY_MINIMUM     (-1000000)
#define GLR_PRIORITY_MAXIMUM     (1000000)

/**
 * @typedef glr_disambiguation_decision_t
 * @brief Decision type after disambiguation
 */
typedef enum {
    GLR_DECISION_SINGLE,            ///< Exactly one winner
    GLR_DECISION_MULTI,             ///< Multiple winners (ambiguity remains)
    GLR_DECISION_NONE,              ///< No valid reductions
    GLR_DECISION_ERROR              ///< Disambiguation failed
} glr_disambiguation_decision_t;

/* ===== Callback Types (for Lua/embedding) ===== */

/**
 * @typedef glr_strategy_priority_callback_t
 * @brief Callback to compute priority for a production
 * @param user_data User-provided context
 * @param context Disambiguation context
 * @param production Production to score
 * @return Priority value
 */
typedef glr_priority_t (*glr_strategy_priority_callback_t)(
    void *user_data,
    const glr_disambiguation_context_t *context,
    const glr_production_t *production
);

/**
 * @typedef glr_strategy_filter_callback_t
 * @brief Callback to filter/accept reductions
 * @param user_data User-provided context
 * @param context Disambiguation context
 * @param result Result structure to populate
 * @return 0 on success, -1 on error
 */
typedef int (*glr_strategy_filter_callback_t)(
    void *user_data,
    const glr_disambiguation_context_t *context,
    glr_disambiguation_result_t *result
);

/**
 * @typedef glr_strategy_hook_callback_t
 * @brief Callback for pre/post resolution hooks
 * @param user_data User-provided context
 * @param context Disambiguation context (or result for post-hook)
 * @return 0 on success, -1 on error
 */
typedef int (*glr_strategy_hook_callback_t)(
    void *user_data,
    void *context_or_result
);

/* ===== Strategy Object Model (Non-VTable) ===== */

/**
 * @struct glr_disambiguation_strategy_t
 * @brief A composable disambiguation strategy
 * 
 * Strategies are lightweight objects that can be combined and composed.
 * Unlike vtables, these are explicit objects that can be inspected,
 * modified, and chained at runtime.
 */
typedef struct glr_disambiguation_strategy {
    char *name;                         ///< Strategy name for debugging
    void *state;                        ///< Strategy-specific state
    glr_strategy_priority_callback_t priority_fn;      ///< Optional priority computation
    glr_strategy_filter_callback_t filter_fn;          ///< Optional reduction filtering
    glr_strategy_hook_callback_t pre_hook_fn;          ///< Optional pre-resolution hook
    glr_strategy_hook_callback_t post_hook_fn;         ///< Optional post-resolution hook
    void (*destroy)(void *state);       ///< Cleanup callback for state
} glr_disambiguation_strategy_t;

/**
 * @struct glr_disambiguation_config_t
 * @brief Configuration for a disambiguator
 * 
 * Holds the list of strategies and their composition order.
 */
typedef struct glr_disambiguation_config {
    glr_disambiguation_strategy_t **strategies;  ///< Array of strategies
    size_t strategy_count;                       ///< Number of strategies
    size_t strategy_capacity;                    ///< Strategy capacity
    glr_priority_t default_priority;             ///< Default priority for productions
    bool allow_ambiguous;                        ///< Whether to allow ambiguity
    bool strict_mode;                            ///< Strict error handling
    void *user_data;                             ///< User data for all strategies
} glr_disambiguation_config_t;

/**
 * @struct glr_disambiguator_t
 * @brief Main disambiguator object
 * 
 * The disambiguator coordinates strategy execution during parsing.
 */
typedef struct glr_disambiguator {
    glr_disambiguation_config_t *config;        ///< Configuration
    void *private_state;                        ///< Implementation-private state
    int reference_count;                        ///< For potential reference counting
} glr_disambiguator_t;

/* ===== Result Utilities ===== */

/**
 * @brief Allocate a disambiguation result with space for N reductions
 * 
 * @param capacity Number of reductions to allocate space for
 * @return Pointer to result, or NULL on memory error
 */
glr_disambiguation_result_t *glr_disambiguation_result_alloc(size_t capacity);

/**
 * @brief Free a disambiguation result
 * 
 * Frees the result structure and the winners array, but NOT the
 * reductions themselves (they remain owned by the forest).
 * 
 * @param result Result to free
 */
void glr_disambiguation_result_free(glr_disambiguation_result_t *result);

/**
 * @brief Create a single-winner result
 * 
 * @param winner The accepted reduction
 * @return Pointer to result, or NULL on memory error
 */
glr_disambiguation_result_t *glr_disambiguation_result_single(glr_reduction_t *winner);

/**
 * @brief Create an empty result (no winners)
 * 
 * @return Pointer to result, or NULL on memory error
 */
glr_disambiguation_result_t *glr_disambiguation_result_empty(void);

/**
 * @brief Create a multi-winner result
 * 
 * @param reductions Array of accepted reductions
 * @param count Number of reductions
 * @return Pointer to result, or NULL on memory error
 */
glr_disambiguation_result_t *glr_disambiguation_result_multi(
    glr_reduction_t **reductions,
    size_t count
);

/**
 * @brief Get the decision type from a result
 * 
 * @param result Result to examine
 * @return Decision type
 */
glr_disambiguation_decision_t glr_disambiguation_result_get_decision(
    const glr_disambiguation_result_t *result
);

/**
 * @brief Copy a result (deep copy of accepted array)
 * 
 * @param source Source result
 * @return New result, or NULL on error
 */
glr_disambiguation_result_t *glr_disambiguation_result_copy(
    const glr_disambiguation_result_t *source
);

/* ===== Strategy Creation and Management ===== */

/**
 * @brief Create a new strategy with given name
 * 
 * @param name Strategy name
 * @return Pointer to strategy, or NULL on memory error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_create(const char *name);

/**
 * @brief Destroy a strategy and its state
 * 
 * @param strategy Strategy to destroy
 */
void glr_disambiguation_strategy_destroy(glr_disambiguation_strategy_t *strategy);

/**
 * @brief Set the priority computation callback for a strategy
 * 
 * @param strategy Strategy to modify
 * @param fn Callback function
 */
void glr_disambiguation_strategy_set_priority_fn(
    glr_disambiguation_strategy_t *strategy,
    glr_strategy_priority_callback_t fn
);

/**
 * @brief Set the filtering callback for a strategy
 * 
 * @param strategy Strategy to modify
 * @param fn Callback function
 */
void glr_disambiguation_strategy_set_filter_fn(
    glr_disambiguation_strategy_t *strategy,
    glr_strategy_filter_callback_t fn
);

/**
 * @brief Set pre-resolution hook callback
 * 
 * @param strategy Strategy to modify
 * @param fn Callback function
 */
void glr_disambiguation_strategy_set_pre_hook(
    glr_disambiguation_strategy_t *strategy,
    glr_strategy_hook_callback_t fn
);

/**
 * @brief Set post-resolution hook callback
 * 
 * @param strategy Strategy to modify
 * @param fn Callback function
 */
void glr_disambiguation_strategy_set_post_hook(
    glr_disambiguation_strategy_t *strategy,
    glr_strategy_hook_callback_t fn
);

/**
 * @brief Clone a strategy (shallow copy)
 * 
 * @param strategy Strategy to clone
 * @return New strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_clone(
    const glr_disambiguation_strategy_t *strategy
);

/**
 * @brief Chain two strategies (creates composite)
 * 
 * The chain executes strategy A first, then strategy B on the result.
 * 
 * @param a First strategy
 * @param b Second strategy
 * @return Composite strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_chain(
    glr_disambiguation_strategy_t *a,
    glr_disambiguation_strategy_t *b
);

/* ===== Configuration Management ===== */

/**
 * @brief Create a new disambiguation configuration
 * 
 * @return Pointer to config, or NULL on memory error
 */
glr_disambiguation_config_t *glr_disambiguation_config_create(void);

/**
 * @brief Destroy a disambiguation configuration
 * 
 * Does NOT destroy the strategies themselves, only the config.
 * 
 * @param config Config to destroy
 */
void glr_disambiguation_config_destroy(glr_disambiguation_config_t *config);

/**
 * @brief Add a strategy to the configuration
 * 
 * @param config Config to modify
 * @param strategy Strategy to add
 * @return 0 on success, -1 on error
 */
int glr_disambiguation_config_add_strategy(
    glr_disambiguation_config_t *config,
    glr_disambiguation_strategy_t *strategy
);

/**
 * @brief Set default priority for the configuration
 * 
 * @param config Config to modify
 * @param priority Default priority value
 */
void glr_disambiguation_config_set_default_priority(
    glr_disambiguation_config_t *config,
    glr_priority_t priority
);

/**
 * @brief Configure whether ambiguity is allowed
 * 
 * @param config Config to modify
 * @param allow Whether to allow ambiguity
 */
void glr_disambiguation_config_allow_ambiguity(
    glr_disambiguation_config_t *config,
    bool allow
);

/**
 * @brief Enable strict mode
 * 
 * In strict mode, disambiguation errors are treated as fatal.
 * 
 * @param config Config to modify
 * @param strict Whether to enable strict mode
 */
void glr_disambiguation_config_set_strict_mode(
    glr_disambiguation_config_t *config,
    bool strict
);

/* ===== Disambiguator Creation and Management ===== */

/**
 * @brief Create a new disambiguator with given configuration
 * 
 * @param config Configuration to use (becomes owned by disambiguator)
 * @return Pointer to disambiguator, or NULL on error
 */
glr_disambiguator_t *glr_disambiguator_create(glr_disambiguation_config_t *config);

/**
 * @brief Create a disambiguator with default configuration
 * 
 * @return Pointer to disambiguator, or NULL on error
 */
glr_disambiguator_t *glr_disambiguator_create_default(void);

/**
 * @brief Destroy a disambiguator
 * 
 * @param disambiguator Disambiguator to destroy
 */
void glr_disambiguator_destroy(glr_disambiguator_t *disambiguator);

/**
 * @brief Run disambiguation on given context
 * 
 * This is the main entry point for disambiguation during parsing.
 * 
 * @param disambiguator Disambiguator to use
 * @param context Disambiguation context
 * @return Result structure, or NULL on error
 */
glr_disambiguation_result_t *glr_disambiguator_disambiguate(
    glr_disambiguator_t *disambiguator,
    const glr_disambiguation_context_t *context
);

/**
 * @brief Get configuration from disambiguator
 * 
 * @param disambiguator Disambiguator
 * @return Configuration, or NULL if none
 */
glr_disambiguation_config_t *glr_disambiguator_get_config(
    glr_disambiguator_t *disambiguator
);

/**
 * @brief Set user data on disambiguator
 * 
 * @param disambiguator Disambiguator
 * @param user_data User data pointer
 */
void glr_disambiguator_set_user_data(
    glr_disambiguator_t *disambiguator,
    void *user_data
);

/**
 * @brief Get user data from disambiguator
 * 
 * @param disambiguator Disambiguator
 * @return User data pointer
 */
void *glr_disambiguator_get_user_data(
    glr_disambiguator_t *disambiguator
);

/* ===== Pre-built Strategies ===== */

/**
 * @brief Create a priority-based strategy
 * 
 * Uses production priorities to select the highest-priority reduction.
 * 
 * @param default_priority Default priority for productions with no explicit priority
 * @return Strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_priority(
    glr_priority_t default_priority
);

/**
 * @brief Create a precedence-based strategy
 * 
 * Resolves ambiguity using operator precedence and associativity.
 * Requires grammar with precedence annotations.
 * 
 * @param default_precedence Default precedence level
 * @return Strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_precedence(
    glr_priority_t default_precedence
);

/**
 * @brief Create a left-associativity strategy
 * 
 * Prefers left-associative reductions for ambiguous operators.
 * 
 * @return Strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_left_assoc(void);

/**
 * @brief Create a right-associativity strategy
 * 
 * Prefers right-associative reductions for ambiguous operators.
 * 
 * @return Strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_right_assoc(void);

/**
 * @brief Create a length-priority strategy
 * 
 * Prefers reductions that consume more input (longer matches).
 * Useful for longest-match lexing.
 * 
 * @return Strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_longest_match(void);

/**
 * @brief Create a production-count strategy
 * 
 * Prefers reductions using more frequently-occurring productions
 * (based on historical statistics).
 * 
 * @return Strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_production_freq(void);

/**
 * @brief Create a semantic predicate strategy
 * 
 * Uses user-provided semantic predicates to filter reductions.
 * 
 * @param predicate_fn Callback to evaluate predicates
 * @return Strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_semantic_predicate(
    glr_strategy_filter_callback_t predicate_fn
);

/**
 * @brief Create a lookahead predicate strategy
 * 
 * Uses syntactic lookahead to validate reductions.
 * 
 * @param lookahead_fn Callback to perform lookahead
 * @return Strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_lookahead(
    glr_strategy_filter_callback_t lookahead_fn
);

/**
 * @brief Create a wildcard strategy (accepts all)
 * 
 * Does not filter any reductions; passes all candidates through.
 * Useful for debugging or when disambiguation is deferred.
 * 
 * @return Strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_wildcard(void);

/* ===== Utility Functions ===== */

/**
 * @brief Get a string description of a decision type
 * 
 * @param decision Decision type
 * @return String description
 */
const char *glr_disambiguation_decision_string(
    glr_disambiguation_decision_t decision
);

/**
 * @brief Get the version string for the disambiguation module
 * 
 * @return Version string
 */
const char *glr_disambiguation_version(void);

/**
 * @brief Log disambiguation context to stderr (debugging)
 * 
 * @param context Context to log
 */
void glr_disambiguation_log_context(
    const glr_disambiguation_context_t *context
);

/**
 * @brief Log disambiguation result to stderr (debugging)
 * 
 * @param result Result to log
 */
void glr_disambiguation_log_result(
    const glr_disambiguation_result_t *result
);

/* ===== Lua/C Integration Helpers ===== */

/**
 * @brief Push a disambiguation result onto a Lua stack
 * 
 * This helper is provided for Lua integration. It expects:
 * - Stack has space for results
 * - Result structure is valid
 * 
 * @param result Result to push
 * @return Number of values pushed
 */
int glr_disambiguation_result_push_lua(
    const glr_disambiguation_result_t *result
);

/**
 * @brief Pop a disambiguation result from a Lua stack
 * 
 * This helper extracts result data from Lua values.
 * 
 * @return Populated result structure, or NULL on error
 */
glr_disambiguation_result_t *glr_disambiguation_result_pop_lua(void);

/**
 * @brief Create a disambiguation strategy from Lua
 * 
 * This helper creates a C strategy from a Lua function.
 * 
 * @param lua_fn Lua function to use as filter
 * @return Strategy, or NULL on error
 */
glr_disambiguation_strategy_t *glr_disambiguation_strategy_from_lua(void *lua_fn);

#ifdef __cplusplus
}
#endif

#endif /* GLR_DISAMBIGUATE_H */
