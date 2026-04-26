// src/glr/cache.c
#include <glr/cache.h>
#include <glr/forest.h>
#include <glr/stack.h>
#include <glr/grammar.h>
#include <glr/serialization.h>

#include <lmdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <openssl/sha.h>

/* Internal cache structure */
struct glr_cache_t {
    MDB_env* env;
    MDB_dbi dbi_forest;
    MDB_dbi dbi_gss;
    MDB_dbi dbi_subtree;
    MDB_dbi dbi_metadata;
    glr_cache_config_t config;
    glr_cache_stats_t stats;
    uint32_t grammar_crc;
};

const glr_cache_config_t GLR_CACHE_DEFAULT_CONFIG = {
    .lmdb_path = ".",
    .map_size = 1024 * 1024 * 1024,  /* 1GB */
    .max_readers = 126,
    .use_async = false,
    .ttl_seconds = 86400              /* 1 day */
};

/* Helper: Convert time_t to 64-bit for storage */
static uint64_t time_to_u64(time_t t) {
    return (uint64_t)t;
}

static time_t u64_to_time(uint64_t u) {
    return (time_t)u;
}

glr_cache_t* glr_cache_open(const glr_cache_config_t* config) {
    glr_cache_t* cache = calloc(1, sizeof(glr_cache_t));
    if (!cache) return NULL;
    
    memcpy(&cache->config, config, sizeof(glr_cache_config_t));
    memset(&cache->stats, 0, sizeof(glr_cache_stats_t));
    
    /* Create LMDB environment */
    int rc = mdb_env_create(&cache->env);
    if (rc != 0) {
        fprintf(stderr, "mdb_env_create: %s\n", mdb_strerror(rc));
        free(cache);
        return NULL;
    }
    
    rc = mdb_env_set_mapsize(cache->env, cache->config.map_size);
    if (rc != 0) {
        fprintf(stderr, "mdb_env_set_mapsize: %s\n", mdb_strerror(rc));
        mdb_env_close(cache->env);
        free(cache);
        return NULL;
    }
    
    rc = mdb_env_set_maxreaders(cache->env, cache->config.max_readers);
    if (rc != 0) {
        fprintf(stderr, "mdb_env_set_maxreaders: %s\n", mdb_strerror(rc));
        mdb_env_close(cache->env);
        free(cache);
        return NULL;
    }
    
    rc = mdb_env_set_maxdbs(cache->env, 4);
    if (rc != 0) {
        fprintf(stderr, "mdb_env_set_maxdbs: %s\n", mdb_strerror(rc));
        mdb_env_close(cache->env);
        free(cache);
        return NULL;
    }
    
    unsigned int env_flags = 0;
    if (!cache->config.use_async) {
        env_flags |= MDB_MAPASYNC;
    }
    
    rc = mdb_env_open(cache->env, cache->config.lmdb_path, env_flags, 0664);
    if (rc != 0) {
        fprintf(stderr, "mdb_env_open: %s\n", mdb_strerror(rc));
        mdb_env_close(cache->env);
        free(cache);
        return NULL;
    }
    
    /* Open databases */
    MDB_txn* txn;
    rc = mdb_txn_begin(cache->env, NULL, 0, &txn);
    if (rc != 0) {
        fprintf(stderr, "mdb_txn_begin: %s\n", mdb_strerror(rc));
        mdb_env_close(cache->env);
        free(cache);
        return NULL;
    }
    
    rc = mdb_dbi_open(txn, "forests", MDB_CREATE, &cache->dbi_forest);
    if (rc != 0) goto cleanup;
    
    rc = mdb_dbi_open(txn, "gss_nodes", MDB_CREATE, &cache->dbi_gss);
    if (rc != 0) goto cleanup;
    
    rc = mdb_dbi_open(txn, "subtrees", MDB_CREATE, &cache->dbi_subtree);
    if (rc != 0) goto cleanup;
    
    rc = mdb_dbi_open(txn, "metadata", MDB_CREATE, &cache->dbi_metadata);
    if (rc != 0) goto cleanup;
    
    mdb_txn_commit(txn);
    
    return cache;
    
cleanup:
    mdb_txn_abort(txn);
    mdb_env_close(cache->env);
    free(cache);
    return NULL;
}

void glr_cache_close(glr_cache_t* cache) {
    if (!cache) return;
    
    if (cache->env) {
        mdb_env_close(cache->env);
    }
    
    free(cache);
}

int glr_cache_sync(glr_cache_t* cache) {
    if (!cache) return -1;
    return mdb_env_sync(cache->env, 1);
}

void glr_cache_compute_hash(const uint8_t* data, size_t len, uint8_t hash[32]) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(hash, &ctx);
}

/* Helper: Pack a cache key into a single integer for indexing */
static uint64_t pack_forest_key(const glr_forest_cache_key_t* key) {
    uint64_t packed = 0;
    /* Use first 8 bytes of hash + grammar_crc + start_symbol */
    memcpy(&packed, key->content_hash, 8);
    packed ^= ((uint64_t)key->grammar_crc << 32);
    packed ^= ((uint64_t)key->start_symbol << 48);
    return packed;
}

static uint64_t pack_gss_key(const glr_gss_cache_key_t* key) {
    return ((uint64_t)key->state_id << 32) | 
           ((uint64_t)key->position << 0) |
           ((uint64_t)key->grammar_crc << 48);
}

static uint64_t pack_subtree_key(const glr_subtree_cache_key_t* key) {
    uint64_t packed = 0;
    memcpy(&packed, key->content_hash, 8);
    packed ^= ((uint64_t)key->production_id << 32);
    packed ^= ((uint64_t)key->grammar_crc << 48);
    return packed;
}

int glr_cache_lookup_forest(glr_cache_t* cache,
                            const glr_forest_cache_key_t* key,
                            glr_forest_t** out_forest) {
    if (!cache || !key || !out_forest) return -1;
    
    uint64_t packed_key = pack_forest_key(key);
    
    MDB_txn* txn;
    MDB_val mdb_key, mdb_data;
    
    int rc = mdb_txn_begin(cache->env, NULL, MDB_RDONLY, &txn);
    if (rc != 0) return -1;
    
    mdb_key.mv_data = &packed_key;
    mdb_key.mv_size = sizeof(packed_key);
    
    rc = mdb_get(txn, cache->dbi_forest, &mdb_key, &mdb_data);
    if (rc == MDB_NOTFOUND) {
        mdb_txn_abort(txn);
        cache->stats.forest_misses++;
        return 0;
    } else if (rc != 0) {
        mdb_txn_abort(txn);
        return -1;
    }
    
    /* Deserialize forest */
    rc = glr_deserialize_forest((const uint8_t*)mdb_data.mv_data, 
                                mdb_data.mv_size, out_forest);
    
    mdb_txn_abort(txn);
    
    if (rc == 0) {
        cache->stats.forest_hits++;
        return 1;
    } else {
        cache->stats.forest_misses++;
        return -1;
    }
}

int glr_cache_store_forest(glr_cache_t* cache,
                           const glr_forest_cache_key_t* key,
                           const glr_forest_t* forest) {
    if (!cache || !key || !forest) return -1;
    
    uint64_t packed_key = pack_forest_key(key);
    
    /* Serialize forest */
    uint8_t* data;
    size_t len;
    if (glr_serialize_forest(forest, &data, &len) < 0) {
        return -1;
    }
    
    MDB_txn* txn;
    MDB_val mdb_key, mdb_data;
    
    int rc = mdb_txn_begin(cache->env, NULL, 0, &txn);
    if (rc != 0) {
        free(data);
        return -1;
    }
    
    mdb_key.mv_data = &packed_key;
    mdb_key.mv_size = sizeof(packed_key);
    mdb_data.mv_data = data;
    mdb_data.mv_size = len;
    
    rc = mdb_put(txn, cache->dbi_forest, &mdb_key, &mdb_data, 0);
    
    free(data);
    
    if (rc != 0) {
        mdb_txn_abort(txn);
        return -1;
    }
    
    /* Store timestamp in metadata */
    char meta_key[64];
    snprintf(meta_key, sizeof(meta_key), "forest_ts:%lu", (unsigned long)packed_key);
    uint64_t timestamp = time_to_u64(time(NULL));
    
    mdb_key.mv_data = meta_key;
    mdb_key.mv_size = strlen(meta_key);
    mdb_data.mv_data = &timestamp;
    mdb_data.mv_size = sizeof(timestamp);
    
    mdb_put(txn, cache->dbi_metadata, &mdb_key, &mdb_data, 0);
    
    mdb_txn_commit(txn);
    cache->stats.forest_count++;
    
    return 0;
}

int glr_cache_lookup_gss_node(glr_cache_t* cache,
                              const glr_gss_cache_key_t* key,
                              glr_stack_node_t** out_node) {
    if (!cache || !key || !out_node) return -1;
    
    uint64_t packed_key = pack_gss_key(key);
    
    MDB_txn* txn;
    MDB_val mdb_key, mdb_data;
    
    int rc = mdb_txn_begin(cache->env, NULL, MDB_RDONLY, &txn);
    if (rc != 0) return -1;
    
    mdb_key.mv_data = &packed_key;
    mdb_key.mv_size = sizeof(packed_key);
    
    rc = mdb_get(txn, cache->dbi_gss, &mdb_key, &mdb_data);
    if (rc == MDB_NOTFOUND) {
        mdb_txn_abort(txn);
        cache->stats.gss_misses++;
        return 0;
    } else if (rc != 0) {
        mdb_txn_abort(txn);
        return -1;
    }
    
    rc = glr_deserialize_stack_node((const uint8_t*)mdb_data.mv_data,
                                   mdb_data.mv_size, out_node);
    
    mdb_txn_abort(txn);
    
    if (rc == 0) {
        cache->stats.gss_hits++;
        return 1;
    } else {
        cache->stats.gss_misses++;
        return -1;
    }
}

int glr_cache_store_gss_node(glr_cache_t* cache,
                             const glr_gss_cache_key_t* key,
                             const glr_stack_node_t* node) {
    if (!cache || !key || !node) return -1;
    
    uint64_t packed_key = pack_gss_key(key);
    
    uint8_t* data;
    size_t len;
    if (glr_serialize_stack_node(node, &data, &len) < 0) {
        return -1;
    }
    
    MDB_txn* txn;
    MDB_val mdb_key, mdb_data;
    
    int rc = mdb_txn_begin(cache->env, NULL, 0, &txn);
    if (rc != 0) {
        free(data);
        return -1;
    }
    
    mdb_key.mv_data = &packed_key;
    mdb_key.mv_size = sizeof(packed_key);
    mdb_data.mv_data = data;
    mdb_data.mv_size = len;
    
    rc = mdb_put(txn, cache->dbi_gss, &mdb_key, &mdb_data, 0);
    
    free(data);
    
    if (rc != 0) {
        mdb_txn_abort(txn);
        return -1;
    }
    
    mdb_txn_commit(txn);
    cache->stats.gss_count++;
    
    return 0;
}

int glr_cache_lookup_subtree(glr_cache_t* cache,
                             const glr_subtree_cache_key_t* key,
                             uint8_t** out_data,
                             size_t* out_len) {
    if (!cache || !key || !out_data || !out_len) return -1;
    
    uint64_t packed_key = pack_subtree_key(key);
    
    MDB_txn* txn;
    MDB_val mdb_key, mdb_data;
    
    int rc = mdb_txn_begin(cache->env, NULL, MDB_RDONLY, &txn);
    if (rc != 0) return -1;
    
    mdb_key.mv_data = &packed_key;
    mdb_key.mv_size = sizeof(packed_key);
    
    rc = mdb_get(txn, cache->dbi_subtree, &mdb_key, &mdb_data);
    if (rc == MDB_NOTFOUND) {
        mdb_txn_abort(txn);
        cache->stats.subtree_misses++;
        return 0;
    } else if (rc != 0) {
        mdb_txn_abort(txn);
        return -1;
    }
    
    *out_data = malloc(mdb_data.mv_size);
    if (!*out_data) {
        mdb_txn_abort(txn);
        return -1;
    }
    
    memcpy(*out_data, mdb_data.mv_data, mdb_data.mv_size);
    *out_len = mdb_data.mv_size;
    
    mdb_txn_abort(txn);
    cache->stats.subtree_hits++;
    
    return 1;
}

int glr_cache_store_subtree(glr_cache_t* cache,
                            const glr_subtree_cache_key_t* key,
                            const uint8_t* data,
                            size_t len) {
    if (!cache || !key || !data) return -1;
    
    uint64_t packed_key = pack_subtree_key(key);
    
    MDB_txn* txn;
    MDB_val mdb_key, mdb_data;
    
    int rc = mdb_txn_begin(cache->env, NULL, 0, &txn);
    if (rc != 0) return -1;
    
    mdb_key.mv_data = &packed_key;
    mdb_key.mv_size = sizeof(packed_key);
    mdb_data.mv_data = (void*)data;
    mdb_data.mv_size = len;
    
    rc = mdb_put(txn, cache->dbi_subtree, &mdb_key, &mdb_data, 0);
    
    if (rc != 0) {
        mdb_txn_abort(txn);
        return -1;
    }
    
    mdb_txn_commit(txn);
    cache->stats.subtree_count++;
    
    return 0;
}

int glr_cache_invalidate_range(glr_cache_t* cache,
                               uint32_t start_byte,
                               uint32_t end_byte) {
    if (!cache) return -1;
    
    /* This would use the dependency tracking system */
    /* For now, it's a stub */
    (void)start_byte;
    (void)end_byte;
    
    return 0;
}

int glr_cache_get_stats(glr_cache_t* cache, glr_cache_stats_t* stats) {
    if (!cache || !stats) return -1;
    
    memcpy(stats, &cache->stats, sizeof(glr_cache_stats_t));
    
    /* Get database sizes */
    MDB_stat mdb_stat;
    mdb_stat.ms_psize = 0;
    
    if (mdb_env_stat(cache->env, &mdb_stat) == 0) {
        stats->cache_size_bytes = mdb_stat.ms_psize * 
                                  (mdb_stat.ms_leaf_pages + mdb_stat.ms_branch_pages);
    }
    
    return 0;
}

int glr_cache_clear(glr_cache_t* cache) {
    if (!cache) return -1;
    
    MDB_txn* txn;
    int rc = mdb_txn_begin(cache->env, NULL, 0, &txn);
    if (rc != 0) return -1;
    
    rc = mdb_drop(txn, cache->dbi_forest, 0);
    if (rc != 0) goto cleanup;
    
    rc = mdb_drop(txn, cache->dbi_gss, 0);
    if (rc != 0) goto cleanup;
    
    rc = mdb_drop(txn, cache->dbi_subtree, 0);
    if (rc != 0) goto cleanup;
    
    mdb_txn_commit(txn);
    
    /* Reset stats */
    memset(&cache->stats, 0, sizeof(glr_cache_stats_t));
    
    return 0;
    
cleanup:
    mdb_txn_abort(txn);
    return -1;
}

int glr_cache_vacuum(glr_cache_t* cache) {
    /* LMDB doesn't support online vacuum */
    /* Just sync for now */
    return glr_cache_sync(cache);
}
