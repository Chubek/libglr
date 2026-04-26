#include <glr/serialization.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SERIAL_VERSION 1

/* Helper: Write data to buffer with automatic expansion */
typedef struct {
    uint8_t* data;
    size_t size;
    size_t capacity;
} write_buffer_t;

static int buffer_init(write_buffer_t* buf, size_t initial_capacity) {
    buf->data = malloc(initial_capacity);
    if (!buf->data) return -1;
    buf->size = 0;
    buf->capacity = initial_capacity;
    return 0;
}

static void buffer_free(write_buffer_t* buf) {
    free(buf->data);
    buf->data = NULL;
    buf->size = 0;
    buf->capacity = 0;
}

static int buffer_write(write_buffer_t* buf, const void* data, size_t len) {
    if (buf->size + len > buf->capacity) {
        size_t new_capacity = buf->capacity * 2;
        while (new_capacity < buf->size + len) {
            new_capacity *= 2;
        }
        uint8_t* new_data = realloc(buf->data, new_capacity);
        if (!new_data) return -1;
        buf->data = new_data;
        buf->capacity = new_capacity;
    }
    memcpy(buf->data + buf->size, data, len);
    buf->size += len;
    return 0;
}

static size_t buffer_tell(write_buffer_t* buf) {
    return buf->size;
}

/* Serialize a single forest node (recursive) */
static int serialize_node_recursive(const glr_forest_node_t* node, 
                                     write_buffer_t* buf) {
    if (!node) return 0;
    
    glr_serialized_node_header_t header;
    header.tag = GLR_SERIAL_TAG_FOREST_NODE;
    header.node_type = node->type;
    header.symbol_id = node->symbol_id;
    header.position = node->position;
    header.child_count = node->child_count;
    header.data_size = 0;  /* User data not serialized for now */
    
    size_t header_pos = buffer_tell(buf);
    if (buffer_write(buf, &header, sizeof(header)) < 0) return -1;
    
    /* Write children offsets (will be filled later) */
    size_t children_offset_pos = buffer_tell(buf);
    header.children_offset = children_offset_pos;
    
    if (node->child_count > 0) {
        uint32_t* child_offsets = calloc(node->child_count, sizeof(uint32_t));
        if (!child_offsets) return -1;
        
        if (buffer_write(buf, child_offsets, node->child_count * sizeof(uint32_t)) < 0) {
            free(child_offsets);
            return -1;
        }
        
        /* Serialize each child and record offset */
        for (size_t i = 0; i < node->child_count; i++) {
            child_offsets[i] = buffer_tell(buf);
            if (serialize_node_recursive(node->children[i], buf) < 0) {
                free(child_offsets);
                return -1;
            }
        }
        
        /* Update child offsets in buffer */
        memcpy(buf->data + children_offset_pos, child_offsets, 
               node->child_count * sizeof(uint32_t));
        free(child_offsets);
    } else {
        header.children_offset = 0;
    }
    
    /* Update header size */
    header.size = buffer_tell(buf) - header_pos;
    memcpy(buf->data + header_pos, &header, sizeof(header));
    
    return 0;
}

int glr_serialize_forest_node(const glr_forest_node_t* node,
                              uint8_t** out_data,
                              size_t* out_len) {
    if (!node || !out_data || !out_len) return -1;
    
    write_buffer_t buf;
    if (buffer_init(&buf, 4096) < 0) return -1;
    
    if (serialize_node_recursive(node, &buf) < 0) {
        buffer_free(&buf);
        return -1;
    }
    
    *out_data = buf.data;
    *out_len = buf.size;
    return 0;
}

int glr_serialize_forest(const glr_forest_t* forest, 
                         uint8_t** out_data, 
                         size_t* out_len) {
    if (!forest || !out_data || !out_len) return -1;
    
    write_buffer_t buf;
    if (buffer_init(&buf, 8192) < 0) return -1;
    
    /* Write header */
    glr_serialized_forest_header_t header;
    header.tag = GLR_SERIAL_TAG_FOREST;
    header.version = SERIAL_VERSION;
    header.node_count = forest->node_count;
    header.edge_count = forest->edge_count;
    
    size_t header_pos = buffer_tell(&buf);
    if (buffer_write(&buf, &header, sizeof(header)) < 0) {
        buffer_free(&buf);
        return -1;
    }
    
    /* Write node position offsets */
    header.nodes_offset = buffer_tell(&buf);
    uint32_t* node_offsets = calloc(forest->node_count, sizeof(uint32_t));
    if (!node_offsets) {
        buffer_free(&buf);
        return -1;
    }
    
    if (buffer_write(&buf, node_offsets, forest->node_count * sizeof(uint32_t)) < 0) {
        free(node_offsets);
        buffer_free(&buf);
        return -1;
    }
    
    /* Serialize all nodes at each position */
    for (size_t pos = 0; pos < forest->node_count; pos++) {
        node_offsets[pos] = buffer_tell(&buf);
        
        glr_forest_node_t* node = forest->nodes[pos];
        uint32_t node_count_at_pos = 0;
        
        /* Count nodes at this position */
        glr_forest_node_t* temp = node;
        while (temp) {
            node_count_at_pos++;
            temp = temp->next;
        }
        
        /* Write node count */
        if (buffer_write(&buf, &node_count_at_pos, sizeof(uint32_t)) < 0) {
            free(node_offsets);
            buffer_free(&buf);
            return -1;
        }
        
        /* Serialize each node in the linked list */
        while (node) {
            if (serialize_node_recursive(node, &buf) < 0) {
                free(node_offsets);
                buffer_free(&buf);
                return -1;
            }
            node = node->next;
        }
    }
    
    /* Update node offsets in header */
    memcpy(buf.data + header.nodes_offset, node_offsets, 
           forest->node_count * sizeof(uint32_t));
    free(node_offsets);
    
    /* Edges are not serialized for now (can be reconstructed) */
    header.edges_offset = 0;
    
    /* Update header */
    header.size = buf.size;
    memcpy(buf.data + header_pos, &header, sizeof(header));
    
    *out_data = buf.data;
    *out_len = buf.size;
    return 0;
}

/* Deserialization */
static int deserialize_node_recursive(const uint8_t* data, size_t len,
                                       size_t offset,
                                       glr_forest_node_t** out_node) {
    if (offset + sizeof(glr_serialized_node_header_t) > len) return -1;
    
    const glr_serialized_node_header_t* header = 
        (const glr_serialized_node_header_t*)(data + offset);
    
    if (header->tag != GLR_SERIAL_TAG_FOREST_NODE) return -1;
    
    glr_forest_node_t* node = calloc(1, sizeof(glr_forest_node_t));
    if (!node) return -1;
    
    node->type = header->node_type;
    node->symbol_id = header->symbol_id;
    node->position = header->position;
    node->child_count = header->child_count;
    node->capacity = header->child_count;
    node->data = NULL;
    node->next = NULL;
    
    if (node->child_count > 0) {
        node->children = calloc(node->child_count, sizeof(glr_forest_node_t*));
        if (!node->children) {
            free(node);
            return -1;
        }
        
        const uint32_t* child_offsets = (const uint32_t*)(data + header->children_offset);
        
        for (size_t i = 0; i < node->child_count; i++) {
            if (deserialize_node_recursive(data, len, child_offsets[i], 
                                          &node->children[i]) < 0) {
                /* Cleanup */
                for (size_t j = 0; j < i; j++) {
                    free(node->children[j]);
                }
                free(node->children);
                free(node);
                return -1;
            }
        }
    }
    
    *out_node = node;
    return 0;
}

int glr_deserialize_forest_node(const uint8_t* data,
                                size_t len,
                                glr_forest_node_t** out_node) {
    if (!data || !out_node) return -1;
    return deserialize_node_recursive(data, len, 0, out_node);
}

int glr_deserialize_forest(const uint8_t* data, 
                           size_t len, 
                           glr_forest_t** out_forest) {
    if (!data || !out_forest || len < sizeof(glr_serialized_forest_header_t)) {
        return -1;
    }
    
    const glr_serialized_forest_header_t* header = 
        (const glr_serialized_forest_header_t*)data;
    
    if (header->tag != GLR_SERIAL_TAG_FOREST || 
        header->version != SERIAL_VERSION) {
        return -1;
    }
    
    glr_forest_t* forest = glr_forest_create();
    if (!forest) return -1;
    
    /* Allocate nodes array */
    if (header->node_count > 0) {
        forest->nodes = calloc(header->node_count, sizeof(glr_forest_node_t*));
        if (!forest->nodes) {
            glr_forest_destroy(forest);
            return -1;
        }
        forest->node_count = header->node_count;
    }
    
    const uint32_t* node_offsets = (const uint32_t*)(data + header->nodes_offset);
    
    /* Deserialize nodes at each position */
    for (size_t pos = 0; pos < header->node_count; pos++) {
        uint32_t offset = node_offsets[pos];
        if (offset + sizeof(uint32_t) > len) {
            glr_forest_destroy(forest);
            return -1;
        }
        
        const uint32_t* node_count_ptr = (const uint32_t*)(data + offset);
        uint32_t node_count_at_pos = *node_count_ptr;
        offset += sizeof(uint32_t);
        
        glr_forest_node_t* prev = NULL;
        for (uint32_t i = 0; i < node_count_at_pos; i++) {
            glr_forest_node_t* node;
            if (deserialize_node_recursive(data, len, offset, &node) < 0) {
                glr_forest_destroy(forest);
                return -1;
            }
            
            if (prev) {
                prev->next = node;
            } else {
                forest->nodes[pos] = node;
            }
            prev = node;
            
            /* Move offset forward */
            const glr_serialized_node_header_t* node_header = 
                (const glr_serialized_node_header_t*)(data + offset);
            offset += node_header->size;
        }
    }
    
    *out_forest = forest;
    return 0;
}

/* GSS node serialization (simplified - stack nodes are simpler) */
int glr_serialize_stack_node(const glr_stack_node_t* node,
                             uint8_t** out_data,
                             size_t* out_len) {
    if (!node || !out_data || !out_len) return -1;
    
    /* For now, just create a minimal serialization */
    /* Real implementation would need to serialize the full GSS structure */
    glr_serialized_gss_node_t header;
    header.tag = GLR_SERIAL_TAG_GSS_NODE;
    header.size = sizeof(header);
    header.state_id = 0;  /* Would need parser state info */
    header.position = 0;
    header.parent_count = 0;
    header.parent_offset = 0;
    
    *out_data = malloc(sizeof(header));
    if (!*out_data) return -1;
    
    memcpy(*out_data, &header, sizeof(header));
    *out_len = sizeof(header);
    
    return 0;
}

int glr_deserialize_stack_node(const uint8_t* data,
                               size_t len,
                               glr_stack_node_t** out_node) {
    if (!data || !out_node || len < sizeof(glr_serialized_gss_node_t)) {
        return -1;
    }
    
    /* Simplified deserialization */
    /* Real implementation would reconstruct the GSS node */
    *out_node = NULL;
    return 0;
}
