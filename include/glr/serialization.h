#ifndef GLR_SERIALIZATION_H
#define GLR_SERIALIZATION_H

#include <glr/forest.h>
#include <glr/stack.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file serialization.h
 * @brief Binary serialization for GLR structures
 * 
 * Provides efficient serialization/deserialization for caching
 * parse forests, GSS nodes, and AST subtrees.
 */

/* Serialization format tags */
#define GLR_SERIAL_TAG_FOREST       0x474C5246  /* "GLRF" */
#define GLR_SERIAL_TAG_GSS_NODE     0x474C5347  /* "GLSG" */
#define GLR_SERIAL_TAG_FOREST_NODE  0x474C4E44  /* "GLND" */

/* Serialized forest node header */
typedef struct __attribute__((packed)) {
    uint32_t tag;              /* NODE_TYPE_* */
    uint32_t size;             /* Total size including header */
    uint32_t node_type;        /* glr_forest_node_type_t */
    int32_t  symbol_id;
    uint64_t position;
    uint32_t child_count;
    uint32_t children_offset;  /* Offset to child indices array */
    uint32_t data_offset;      /* Offset to node data (if any) */
    uint32_t data_size;        /* Size of node data */
} glr_serialized_node_header_t;

/* Serialized GSS node */
typedef struct __attribute__((packed)) {
    uint32_t tag;
    uint32_t size;
    uint32_t state_id;
    uint64_t position;
    uint32_t parent_count;
    uint32_t parent_offset;    /* Offset to parent indices */
} glr_serialized_gss_node_t;

/* Serialized forest header */
typedef struct __attribute__((packed)) {
    uint32_t tag;              /* GLR_SERIAL_TAG_FOREST */
    uint32_t version;          /* Format version */
    uint32_t size;             /* Total size */
    uint64_t node_count;       /* Number of node positions */
    uint64_t edge_count;       /* Number of edge positions */
    uint32_t nodes_offset;     /* Offset to nodes array */
    uint32_t edges_offset;     /* Offset to edges array */
} glr_serialized_forest_header_t;

/**
 * Serialize a parse forest to binary format
 * 
 * @param forest Forest to serialize
 * @param out_data Output buffer (allocated by function, must be freed)
 * @param out_len Output buffer length
 * @return 0 on success, -1 on error
 */
int glr_serialize_forest(const glr_forest_t* forest, 
                         uint8_t** out_data, 
                         size_t* out_len);

/**
 * Deserialize a parse forest from binary format
 * 
 * @param data Input buffer
 * @param len Input buffer length
 * @param out_forest Output forest (must be freed with glr_forest_destroy)
 * @return 0 on success, -1 on error
 */
int glr_deserialize_forest(const uint8_t* data, 
                           size_t len, 
                           glr_forest_t** out_forest);

/**
 * Serialize a GSS node to binary format
 * 
 * @param node GSS node to serialize
 * @param out_data Output buffer (allocated by function, must be freed)
 * @param out_len Output buffer length
 * @return 0 on success, -1 on error
 */
int glr_serialize_stack_node(const glr_stack_node_t* node,
                             uint8_t** out_data,
                             size_t* out_len);

/**
 * Deserialize a GSS node from binary format
 * 
 * @param data Input buffer
 * @param len Input buffer length
 * @param out_node Output GSS node (must be freed)
 * @return 0 on success, -1 on error
 */
int glr_deserialize_stack_node(const uint8_t* data,
                               size_t len,
                               glr_stack_node_t** out_node);

/**
 * Serialize a single forest node
 * 
 * @param node Node to serialize
 * @param out_data Output buffer (allocated by function, must be freed)
 * @param out_len Output buffer length
 * @return 0 on success, -1 on error
 */
int glr_serialize_forest_node(const glr_forest_node_t* node,
                              uint8_t** out_data,
                              size_t* out_len);

/**
 * Deserialize a single forest node
 * 
 * @param data Input buffer
 * @param len Input buffer length
 * @param out_node Output node (must be freed)
 * @return 0 on success, -1 on error
 */
int glr_deserialize_forest_node(const uint8_t* data,
                                size_t len,
                                glr_forest_node_t** out_node);

#ifdef __cplusplus
}
#endif

#endif /* GLR_SERIALIZATION_H */
