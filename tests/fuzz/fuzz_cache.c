#include <glr/cache.h>
#include <glr/forest.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __AFL_FUZZ_TESTCASE_LEN
__AFL_FUZZ_INIT();
#endif

#define CACHE_DIR "/tmp/libglr_fuzz_cache"

int main(int argc, char** argv) {
#ifdef __AFL_FUZZ_TESTCASE_LEN
    __AFL_INIT();
    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(10000)) {
        int len = __AFL_FUZZ_TESTCASE_LEN;
#else
    unsigned char *buf = NULL;
    int len = 0;
    
    if (argc > 1) {
        FILE* f = fopen(argv[1], "rb");
        if (!f) return 1;
        
        fseek(f, 0, SEEK_END);
        len = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        buf = malloc(len);
        if (!buf) {
            fclose(f);
            return 1;
        }
        
        fread(buf, 1, len, f);
        fclose(f);
    } else {
        return 1;
    }
#endif

    if (len < 4) {
#ifndef __AFL_FUZZ_TESTCASE_LEN
        free(buf);
#endif
        return 0;
    }

    system("rm -rf " CACHE_DIR);
    mkdir(CACHE_DIR, 0755);

    glr_cache_config_t config = GLR_CACHE_DEFAULT_CONFIG;
    config.mdbx_path = CACHE_DIR;
    config.map_size = 10 * 1024 * 1024;
    
    glr_cache_t* cache = glr_cache_open(&config);
    if (!cache) {
#ifndef __AFL_FUZZ_TESTCASE_LEN
        free(buf);
#endif
        return 0;
    }

    uint8_t hash[32];
    glr_cache_compute_hash(buf, len, hash);

    glr_forest_cache_key_t key;
    memcpy(key.content_hash, hash, 32);
    key.grammar_crc = 0;
    key.start_symbol = 1;

    glr_forest_t* forest = glr_forest_create();
    if (forest) {
        glr_cache_store_forest(cache, &key, forest);
        
        glr_forest_t* retrieved = NULL;
        glr_cache_lookup_forest(cache, &key, &retrieved);
        
        if (retrieved) {
            glr_forest_free(retrieved);
        }
        
        glr_forest_free(forest);
    }

    if (len >= 8) {
        uint32_t state = *(uint32_t*)buf;
        uint32_t pos = *(uint32_t*)(buf + 4);
        
        glr_stack_node_t node;
        memset(&node, 0, sizeof(node));
        node.state = state;
        
        glr_cache_store_gss_node(cache, state, pos, &node);
        
        glr_stack_node_t* retrieved = NULL;
        glr_cache_lookup_gss_node(cache, state, pos, &retrieved);
        
        if (retrieved) {
            free(retrieved);
        }
    }

    glr_cache_close(cache);

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif

#ifdef __AFL_FUZZ_TESTCASE_LEN
    }
#endif

    return 0;
}
