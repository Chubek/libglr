#include <glr/forest_merge.h>
#include <glr/forest.h>
#include <stdlib.h>
#include <string.h>

/**
 * Merge three forests into one continuous parse
 * 
 * This is the core of incremental parsing. We have:
 * - left: unchanged prefix parse
 * - middle: newly parsed changed region
 * - right: unchanged suffix parse
 * 
 * We need to combine them into a single coherent forest.
 */
int glr_forest_merge(glr_parser_t* parser,
                     const glr_forest_t* left,
                     const glr_forest_t* middle,
                     const glr_forest_t* right,
                     glr_forest_t** out) {
    if (!parser || !out) return -1;
    
    /* If any component is NULL, handle gracefully */
    if (!left && !middle && !right) {
        *out = glr_forest_create();
        return *out ? 0 : -1;
    }
    
    /* If only middle exists, just return a copy */
    if (!left && !right && middle) {
        /* For now, create a new forest and copy middle */
        glr_forest_t* result = glr_forest_create();
        if (!result) return -1;
        
        /* Deep copy middle forest */
        /* This is simplified - real implementation would properly copy */
        result->node_count = middle->node_count;
        result->edge_count = middle->edge_count;
        
        if (middle->node_count > 0) {
            result->nodes = calloc(middle->node_count, sizeof(glr_forest_node_t*));
            if (!result->nodes) {
                glr_forest_destroy(result);
                return -1;
            }
            /* Copy node pointers (shallow copy for now) */
            memcpy(result->nodes, middle->nodes, 
                   middle->node_count * sizeof(glr_forest_node_t*));
        }
        
        *out = result;
        return 0;
    }
    
    /* Full merge: left + middle + right */
    glr_forest_t* result = glr_forest_create();
    if (!result) return -1;
    
    /* Calculate total size */
    size_t total_nodes = 0;
    if (left) total_nodes += left->node_count;
    if (middle) total_nodes += middle->node_count;
    if (right) total_nodes += right->node_count;
    
    if (total_nodes > 0) {
        result->nodes = calloc(total_nodes, sizeof(glr_forest_node_t*));
        if (!result->nodes) {
            glr_forest_destroy(result);
            return -1;
        }
        result->node_count = total_nodes;
    }
    
    size_t offset = 0;
    
    /* Copy left forest */
    if (left && left->node_count > 0) {
        memcpy(result->nodes + offset, left->nodes,
               left->node_count * sizeof(glr_forest_node_t*));
        offset += left->node_count;
    }
    
    /* Copy middle forest */
    if (middle && middle->node_count > 0) {
        memcpy(result->nodes + offset, middle->nodes,
               middle->node_count * sizeof(glr_forest_node_t*));
        offset += middle->node_count;
    }
    
    /* Copy right forest (with position adjustment) */
    if (right && right->node_count > 0) {
        /* Right forest positions need to be adjusted based on the edit */
        /* For now, just copy - real implementation would adjust positions */
        memcpy(result->nodes + offset, right->nodes,
               right->node_count * sizeof(glr_forest_node_t*));
        offset += right->node_count;
    }
    
    /* Handle edges similarly */
    size_t total_edges = 0;
    if (left) total_edges += left->edge_count;
    if (middle) total_edges += middle->edge_count;
    if (right) total_edges += right->edge_count;
    
    if (total_edges > 0) {
        result->edges = calloc(total_edges, sizeof(glr_forest_edge_t*));
        if (!result->edges) {
            glr_forest_destroy(result);
            return -1;
        }
        result->edge_count = total_edges;
        
        offset = 0;
        if (left && left->edge_count > 0) {
            memcpy(result->edges + offset, left->edges,
                   left->edge_count * sizeof(glr_forest_edge_t*));
            offset += left->edge_count;
        }
        if (middle && middle->edge_count > 0) {
            memcpy(result->edges + offset, middle->edges,
                   middle->edge_count * sizeof(glr_forest_edge_t*));
            offset += middle->edge_count;
        }
        if (right && right->edge_count > 0) {
            memcpy(result->edges + offset, right->edges,
                   right->edge_count * sizeof(glr_forest_edge_t*));
        }
    }
    
    *out = result;
    return 0;
}

/**
 * Adjust forest node positions after an edit
 * 
 * When text is inserted or deleted, positions in the right forest
 * need to be adjusted by the delta.
 */
int glr_forest_adjust_positions(glr_forest_t* forest,
                                 size_t start_pos,
                                 ssize_t delta) {
    if (!forest) return -1;
    
    /* Adjust all node positions >= start_pos by delta */
    for (size_t pos = 0; pos < forest->node_count; pos++) {
        glr_forest_node_t* node = forest->nodes[pos];
        while (node) {
            if (node->position >= start_pos) {
                if (delta < 0 && node->position < (size_t)(-delta)) {
                    /* Position would become negative - error */
                    return -1;
                }
                node->position += delta;
            }
            node = node->next;
        }
    }
    
    /* Adjust edge positions similarly */
    for (size_t pos = 0; pos < forest->edge_count; pos++) {
        glr_forest_edge_t* edge = forest->edges[pos];
        while (edge) {
            if (edge->start_position >= start_pos) {
                if (delta < 0 && edge->start_position < (size_t)(-delta)) {
                    return -1;
                }
                edge->start_position += delta;
            }
            if (edge->end_position >= start_pos) {
                if (delta < 0 && edge->end_position < (size_t)(-delta)) {
                    return -1;
                }
                edge->end_position += delta;
            }
            edge = edge->next;
        }
    }
    
    return 0;
}
