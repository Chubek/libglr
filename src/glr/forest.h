#ifndef GLR_FOREST_H
#define GLR_FOREST_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @file forest.h
 * @brief SPPF (Shared Parse Forest) data structure
 * 
 * This module provides the SPPF implementation for GLR parsers.
 * The SPPF is a compact representation of all parse trees,
 * sharing common substructures to handle ambiguity efficiently.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef glr_forest_node_type_t
 * @brief Type of SPPF node
 */
typedef enum {
    GLR_NODE_TERMINAL,    ///< Terminal node
    GLR_NODE_NONTERMINAL, ///< Non-terminal (split) node
    GLR_NODE_CONSTRUCTOR  ///< Constructor node
} glr_forest_node_type_t;

/**
 * @typedef glr_forest_node_t
 * @brief A node in the SPPF
 * 
 * Nodes represent either terminal symbols, non-terminal splits,
 * or constructor nodes that combine child nodes.
 */
typedef struct glr_forest_node {
    glr_forest_node_type_t type;      ///< Node type
    int symbol_id;                    ///< Symbol ID (terminal or non-terminal)
    size_t position;                  ///< Input position
    struct glr_forest_node **children; ///< Child nodes (for non-terminals)
    size_t child_count;               ///< Number of children
    size_t capacity;                  ///< Child capacity
    void *data;                       ///< Node-specific data
    struct glr_forest_node *next;     ///< Next sibling in same position
} glr_forest_node_t;

/**
 * @typedef glr_forest_edge_t
 * @brief An edge in the SPPF
 * 
 * Edges connect nodes across different positions in the input.
 */
typedef struct glr_forest_edge {
    int nonterminal_id;       ///< Non-terminal on left-hand side
    size_t start_position;    ///< Starting position
    size_t end_position;      ///< Ending position
    struct glr_forest_edge *next; ///< Next edge at same position
} glr_forest_edge_t;

/**
 * @typedef glr_forest_t
 * @brief Complete SPPF container
 * 
 * Manages all nodes and edges in the shared parse forest.
 */
typedef struct {
    glr_forest_node_t **nodes;      ///< All nodes indexed by position
    size_t node_count;              ///< Number of positions
    glr_forest_edge_t **edges;      ///< All edges indexed by position
    size_t edge_count;              ///< Number of positions with edges
} glr_forest_t;

/**
 * @brief Create a new empty forest
 * 
 * @return Pointer to new forest, or NULL on failure
 */
glr_forest_t *glr_forest_create(void);

/**
 * @brief Destroy a forest and free all associated memory
 * 
 * @param forest Pointer to forest to destroy
 */
void glr_forest_destroy(glr_forest_t *forest);

/**
 * @brief Get or create a node at a specific position
 * 
 * @param forest Pointer to forest
 * @param type Node type
 * @param symbol_id Symbol ID
 * @param position Input position
 * @return Pointer to node, or NULL on failure
 */
glr_forest_node_t *glr_forest_get_node(glr_forest_t *forest,
                                       glr_forest_node_type_t type,
                                       int symbol_id, size_t position);

/**
 * @brief Add a child to a non-terminal node
 * 
 * @param parent Parent node
 * @param child Child node
 * @return 0 on success, -1 on failure
 */
int glr_forest_add_child(glr_forest_node_t *parent, glr_forest_node_t *child);

/**
 * @brief Get children of a non-terminal node
 * 
 * @param node Non-terminal node
 * @return Array of children, or NULL if invalid
 */
glr_forest_node_t **glr_forest_get_children(glr_forest_node_t *node);

/**
 * @brief Add an edge to the forest
 * 
 * @param forest Pointer to forest
 * @param edge Edge to add
 * @return 0 on success, -1 on failure
 */
int glr_forest_add_edge(glr_forest_t *forest, glr_forest_edge_t *edge);

/**
 * @brief Get edges at a specific position
 * 
 * @param forest Pointer to forest
 * @param position Input position
 * @return List of edges at position, or NULL
 */
glr_forest_edge_t *glr_forest_get_edges(glr_forest_t *forest, size_t position);

/**
 * @brief Check if a node is a terminal
 * 
 * @param node Pointer to node
 * @return true if terminal, false otherwise
 */
static inline bool glr_forest_node_is_terminal(glr_forest_node_t *node) {
    return node != NULL && node->type == GLR_NODE_TERMINAL;
}

/**
 * @brief Check if a node is a non-terminal
 * 
 * @param node Pointer to node
 * @return true if non-terminal, false otherwise
 */
static inline bool glr_forest_node_is_nonterminal(glr_forest_node_t *node) {
    return node != NULL && node->type == GLR_NODE_NONTERMINAL;
}

#ifdef __cplusplus
}
#endif

#endif /* GLR_FOREST_H */
