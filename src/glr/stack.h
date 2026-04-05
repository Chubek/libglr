#ifndef GLR_STACK_H
#define GLR_STACK_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @file stack.h
 * @brief DAG-based stack for GLR parser
 *
 * This module provides a DAG-based stack implementation for GLR parsers.
 * The stack supports forking, which is essential for handling ambiguity
 * in GLR parsing. Each stack node can have multiple children (forks),
 * and the structure is shared across different parse paths.
 */

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @typedef glr_stack_node_t
   * @brief A node in the DAG-based stack
   *
   * Each node represents a stack frame and can fork into multiple
   * child nodes to represent different parse paths.
   */
  typedef struct glr_stack_node glr_stack_node_t;

  /**
   * @typedef glr_stack_t
   * @brief DAG-based stack container
   *
   * Manages the stack of parse states and handles forking operations.
   */
  typedef struct
  {
    glr_stack_node_t *root; ///< Stack root node
    size_t height;          ///< Current stack height
    void **states;          ///< Array of parser states
    size_t capacity;        ///< Stack capacity
  } glr_stack_t;

  /**
   * @brief Create a new empty stack
   *
   * @return Pointer to new stack, or NULL on failure
   */
  glr_stack_t *glr_stack_create (void);

  /**
   * @brief Destroy a stack and free all associated memory
   *
   * @param stack Pointer to stack to destroy
   */
  void glr_stack_destroy (glr_stack_t *stack);

  /**
   * @brief Fork the stack at the current height
   *
   * Creates a copy of the stack from the root to the specified height.
   * The original stack is not modified.
   *
   * @param stack Pointer to stack to fork
   * @param height Height at which to fork (0 = root)
   * @return Pointer to new forked stack, or NULL on failure
   */
  glr_stack_t *glr_stack_fork (glr_stack_t *stack, size_t height);

  /**
   * @brief Push a state onto the stack
   *
   * @param stack Pointer to stack
   * @param state Pointer to parser state to push
   * @return 0 on success, -1 on failure
   */
  int glr_stack_push (glr_stack_t *stack, void *state);

  /**
   * @brief Pop a state from the stack
   *
   * @param stack Pointer to stack
   * @return Popped state pointer, or NULL on failure/empty stack
   */
  void *glr_stack_pop (glr_stack_t *stack);

  /**
   * @brief Peek at the top state without removing it
   *
   * @param stack Pointer to stack
   * @return Top state pointer, or NULL if empty
   */
  void *glr_stack_peek (glr_stack_t *stack);

  /**
   * @brief Get the state at a specific height
   *
   * @param stack Pointer to stack
   * @param height Height to get state from (0 = bottom)
   * @return State pointer, or NULL if invalid height
   */
  void *glr_stack_get (glr_stack_t *stack, size_t height);

  /**
   * @brief Get the current stack height
   *
   * @param stack Pointer to stack
   * @return Current height
   */
  size_t glr_stack_height (glr_stack_t *stack);

  /**
   * @brief Check if the stack is empty
   *
   * @param stack Pointer to stack
   * @return true if empty, false otherwise
   */
  static inline bool
  glr_stack_empty (glr_stack_t *stack)
  {
    return stack == NULL || stack->height == 0;
  }

  /**
   * @brief Check if the stack is full
   *
   * @param stack Pointer to stack
   * @return true if at capacity, false otherwise
   */
  static inline bool
  glr_stack_full (glr_stack_t *stack)
  {
    return stack != NULL && stack->height >= stack->capacity;
  }

#ifdef __cplusplus
}
#endif

#endif /* GLR_STACK_H */
