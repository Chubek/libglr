// src/glr/parser-incr.c
#include <glr/parser.h>
#include <glr/cache.h>
#include <glr/diff.h>
#include <glr/forest-merge.h>
#include <glr/dependency.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>

#ifdef HAVE_LMDB

/* Extended parser structure with cache support */
typedef struct {
    glr_cache_t* cache;
    glr_forest_t* previous_forest;
    char* previous_content;
    size_t previous_len;
    uint8_t previous_hash[32];
} glr_parser_cache_ctx_t;

/* Global cache context (in real implementation, this would be in parser struct) */
static glr_parser_cache_ctx_t* get_cache_ctx(glr_parser_t* parser) {
    /* This is a simplified approach - real implementation would store this in parser */
    static glr_parser_cache_ctx_t ctx = {0};
    (void)parser;
    return &ctx;
}

void glr_parser_set_cache(glr_parser_t* parser, struct glr_cache_t* cache) {
    if (!parser) return;
    glr_parser_cache_ctx_t* ctx = get_cache_ctx(parser);
    ctx->cache = cache;
}

struct glr_cache_t* glr_parser_get_cache(const glr_parser_t* parser) {
    if (!parser) return NULL;
    glr_parser_cache_ctx_t* ctx = get_cache_ctx((glr_parser_t*)parser);
    return ctx->cache;
}

#endif /* HAVE_LMDB */

int glr_parser_parse_incremental(glr_parser_t* parser,
                                  const glr_forest_t* old_forest,
                                  const char* old_content,
                                  size_t old_len,
                                  const char* new_content,
                                  size_t new_len,
                                  size_t edit_start,
                                  size_t edit_end,
                                  glr_forest_t** out_forest) {
    if (!parser || !new_content || !out_forest) return -1;
    
    /* If no old content or no old forest, do full parse */
    if (!old_content || !old_forest) {
        glr_parse_result_t result = glr_parse(parser, new_content, new_len);
        if (result.error != GLR_PARSE_SUCCESS) {
            return -1;
        }
        *out_forest = result.forest;
        return 0;
    }
    
    /* Compute edit if not provided */
    glr_edit_t edit;
    if (edit_start == 0 && edit_end == 0) {
        if (glr_compute_edit(old_content, old_len, new_content, new_len, &edit) < 0) {
            return -1;
        }
        edit_start = edit.old_start;
        edit_end = edit.old_end;
    } else {
        /* Use provided edit bounds */
        edit.old_start = edit_start;
        edit.old_end = edit_end;
        edit.new_start = edit_start;
        edit.new_end = edit_start + (new_len - old_len) + (edit_end - edit_start);
    }
    
    /* Check if edit is empty */
    if (edit_start == edit_end && old_len == new_len) {
        /* No change - return copy of old forest */
        *out_forest = (glr_forest_t*)old_forest;  /* Simplified */
        return 0;
    }
    
#ifdef HAVE_LMDB
    glr_cache_t* cache = glr_parser_get_cache(parser);
    glr_forest_t* left_forest = NULL;
    glr_forest_t* right_forest = NULL;
    
    if (cache) {
        /* Try to get cached left subtree (unchanged prefix) */
        if (edit_start > 0) {
            uint8_t left_hash[32];
            glr_cache_compute_hash((const uint8_t*)new_content, edit_start, left_hash);
            
            glr_forest_cache_key_t left_key;
            memcpy(left_key.content_hash, left_hash, 32);
            left_key.grammar_crc = 0;  /* Would need real grammar CRC */
            left_key.start_symbol = 0;
            
            glr_cache_lookup_forest(cache, &left_key, &left_forest);
        }
        
        /* Try to get cached right subtree (unchanged suffix) */
        size_t right_start = edit.new_end;
        if (right_start < new_len) {
            uint8_t right_hash[32];
            glr_cache_compute_hash((const uint8_t*)new_content + right_start,
                                  new_len - right_start, right_hash);
            
            glr_forest_cache_key_t right_key;
            memcpy(right_key.content_hash, right_hash, 32);
            right_key.grammar_crc = 0;
            right_key.start_symbol = 0;
            
            glr_cache_lookup_forest(cache, &right_key, &right_forest);
        }
    }
#else
    glr_forest_t* left_forest = NULL;
    glr_forest_t* right_forest = NULL;
#endif
    
    /* Parse the changed region */
    glr_forest_t* middle_forest = NULL;
    size_t changed_start = edit.new_start;
    size_t changed_len = edit.new_end - edit.new_start;
    
    if (changed_len > 0) {
        char* changed_region = malloc(changed_len + 1);
        if (!changed_region) return -1;
        
        memcpy(changed_region, new_content + changed_start, changed_len);
        changed_region[changed_len] = '\0';
        
        glr_parse_result_t result = glr_parse(parser, changed_region, changed_len);
        free(changed_region);
        
        if (result.error != GLR_PARSE_SUCCESS) {
            return -1;
        }
        
        middle_forest = result.forest;
    }
    
    /* Merge forests: left + middle + right */
    int rc = glr_forest_merge(parser, left_forest, middle_forest, right_forest, out_forest);
    
    if (rc != 0) {
        if (middle_forest) glr_forest_destroy(middle_forest);
        return -1;
    }
    
#ifdef HAVE_LMDB
    /* Store result in cache */
    if (cache && rc == 0 && *out_forest) {
        glr_forest_cache_key_t key;
        uint8_t full_hash[32];
        glr_cache_compute_hash((const uint8_t*)new_content, new_len, full_hash);
        memcpy(key.content_hash, full_hash, 32);
        key.grammar_crc = 0;
        key.start_symbol = 0;
        
        glr_cache_store_forest(cache, &key, *out_forest);
        
        /* Store left and right subtrees if they were newly parsed */
        if (!left_forest && edit_start > 0) {
            uint8_t left_hash[32];
            glr_cache_compute_hash((const uint8_t*)new_content, edit_start, left_hash);
            
            glr_forest_cache_key_t left_key;
            memcpy(left_key.content_hash, left_hash, 32);
            left_key.grammar_crc = 0;
            left_key.start_symbol = 0;
            
            /* Would extract left subtree from merged forest and cache it */
        }
        
        if (!right_forest && edit.new_end < new_len) {
            /* Similar for right subtree */
        }
    }
#endif
    
    return 0;
}

#ifdef HAVE_LMDB

int glr_parser_enable_incremental(glr_parser_t* parser, const char* cache_path) {
    if (!parser || !cache_path) return -1;
    
    glr_cache_config_t config = GLR_CACHE_DEFAULT_CONFIG;
    config.lmdb_path = cache_path;
    
    glr_cache_t* cache = glr_cache_open(&config);
    if (!cache) return -1;
    
    glr_parser_set_cache(parser, cache);
    return 0;
}

void glr_parser_disable_incremental(glr_parser_t* parser) {
    if (!parser) return;
    
    glr_cache_t* cache = glr_parser_get_cache(parser);
    if (cache) {
        glr_cache_close(cache);
        glr_parser_set_cache(parser, NULL);
    }
}

int glr_parser_get_cache_stats(glr_parser_t* parser, struct glr_cache_stats_t* stats) {
    if (!parser || !stats) return -1;
    
    glr_cache_t* cache = glr_parser_get_cache(parser);
    if (!cache) return -1;
    
    return glr_cache_get_stats(cache, (glr_cache_stats_t*)stats);
}

#endif /* HAVE_LMDB */
