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
 * @brief Binary serialization for parse forests and graph-structured stack nodes.
 *
 * The cache layer stores parser artifacts as compact byte streams. This header
 * defines the stable wire headers and conversion helpers used by @ref cache.h
 * and incremental parsing code. Serialized data is allocated by the serializer;
 * callers free returned buffers with the C library allocator used by libglr.
 *
 * @see cache.h
 * @see forest.h
 * @see stack.h
 */

/** @def GLR_SERIAL_TAG_FOREST
 *  @brief Four-byte magic tag identifying a serialized forest payload ("GLRF"). */
#define GLR_SERIAL_TAG_FOREST       0x474C5246

/** @def GLR_SERIAL_TAG_GSS_NODE
 *  @brief Four-byte magic tag identifying a serialized GSS node payload ("GLSG"). */
#define GLR_SERIAL_TAG_GSS_NODE     0x474C5347

/** @def GLR_SERIAL_TAG_FOREST_NODE
 *  @brief Four-byte magic tag identifying a serialized single SPPF node payload ("GLND"). */
#define GLR_SERIAL_TAG_FOREST_NODE  0x474C4E44

/**
 * @struct glr_serialized_node_header_t
 * @brief Packed header placed before a serialized @ref glr_forest_node_t.
 */
typedef struct __attribute__((packed)) {
    uint32_t tag;             /**< Format tag for node records. */
    uint32_t size;            /**< Total record size including this header. */
    uint32_t node_type;       /**< Stored @ref glr_forest_node_type_t value. */
    int32_t  symbol_id;       /**< Grammar symbol represented by the node. */
    uint64_t position;        /**< Input position associated with the node. */
    uint32_t child_count;     /**< Number of child indices following the header. */
    uint32_t children_offset; /**< Byte offset to the child-index array. */
    uint32_t data_offset;     /**< Byte offset to optional node payload data. */
    uint32_t data_size;       /**< Size of optional node payload data in bytes. */
} glr_serialized_node_header_t;

/**
 * @struct glr_serialized_gss_node_t
 * @brief Packed record for one graph-structured stack node.
 */
typedef struct __attribute__((packed)) {
    uint32_t tag;           /**< @ref GLR_SERIAL_TAG_GSS_NODE. */
    uint32_t size;          /**< Total record size including this header. */
    uint32_t state_id;      /**< Parser automaton state stored in the stack node. */
    uint64_t position;      /**< Input byte position associated with the state. */
    uint32_t parent_count;  /**< Number of parent references in the record. */
    uint32_t parent_offset; /**< Byte offset to serialized parent indices. */
} glr_serialized_gss_node_t;

/**
 * @struct glr_serialized_forest_header_t
 * @brief Packed top-level header for a serialized parse forest.
 */
typedef struct __attribute__((packed)) {
    uint32_t tag;          /**< @ref GLR_SERIAL_TAG_FOREST. */
    uint32_t version;      /**< Serialization format version. */
    uint32_t size;         /**< Total payload size in bytes. */
    uint64_t node_count;   /**< Number of serialized node records. */
    uint64_t edge_count;   /**< Number of serialized edge records. */
    uint32_t nodes_offset; /**< Byte offset to serialized nodes. */
    uint32_t edges_offset; /**< Byte offset to serialized edges. */
} glr_serialized_forest_header_t;

/**
 * @brief Serialize a parse forest to binary format.
 * @param forest Forest to serialize.
 * @param out_data Output buffer allocated by the function.
 * @param out_len Output buffer length in bytes.
 * @return 0 on success, -1 on invalid input or serialization failure.
 */
int glr_serialize_forest(const glr_forest_t* forest, uint8_t** out_data, size_t* out_len);

/**
 * @brief Deserialize a parse forest from binary format.
 * @param data Input buffer produced by @ref glr_serialize_forest.
 * @param len Input buffer length in bytes.
 * @param out_forest Output forest owned by the caller on success.
 * @return 0 on success, -1 on malformed input or allocation failure.
 */
int glr_deserialize_forest(const uint8_t* data, size_t len, glr_forest_t** out_forest);

/**
 * @brief Serialize a graph-structured stack node.
 * @param node Stack node to serialize.
 * @param out_data Output buffer allocated by the function.
 * @param out_len Output buffer length in bytes.
 * @return 0 on success, -1 on invalid input or serialization failure.
 */
int glr_serialize_stack_node(const glr_stack_node_t* node, uint8_t** out_data, size_t* out_len);

/**
 * @brief Deserialize a graph-structured stack node.
 * @param data Input buffer produced by @ref glr_serialize_stack_node.
 * @param len Input buffer length in bytes.
 * @param out_node Output stack node owned by the caller on success.
 * @return 0 on success, -1 on malformed input or allocation failure.
 */
int glr_deserialize_stack_node(const uint8_t* data, size_t len, glr_stack_node_t** out_node);

/**
 * @brief Serialize a single SPPF node.
 * @param node Forest node to serialize.
 * @param out_data Output buffer allocated by the function.
 * @param out_len Output buffer length in bytes.
 * @return 0 on success, -1 on invalid input or serialization failure.
 */
int glr_serialize_forest_node(const glr_forest_node_t* node, uint8_t** out_data, size_t* out_len);

/**
 * @brief Deserialize a single SPPF node.
 * @param data Input buffer produced by @ref glr_serialize_forest_node.
 * @param len Input buffer length in bytes.
 * @param out_node Output node owned by the caller on success.
 * @return 0 on success, -1 on malformed input or allocation failure.
 */
int glr_deserialize_forest_node(const uint8_t* data, size_t len, glr_forest_node_t** out_node);

#ifdef __cplusplus
}
#endif

#endif /* GLR_SERIALIZATION_H */
