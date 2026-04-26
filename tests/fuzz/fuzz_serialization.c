#include <glr/serialization.h>
#include <glr/forest.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __AFL_FUZZ_TESTCASE_LEN
__AFL_FUZZ_INIT();
#endif

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

    if (len < 8) {
#ifndef __AFL_FUZZ_TESTCASE_LEN
        free(buf);
#endif
        return 0;
    }

    glr_forest_t* forest = glr_deserialize_forest(buf, len);
    if (forest) {
        size_t out_size = 0;
        uint8_t* serialized = glr_serialize_forest(forest, &out_size);
        
        if (serialized) {
            free(serialized);
        }
        
        glr_forest_free(forest);
    }

    glr_stack_node_t* node = glr_deserialize_gss_node(buf, len);
    if (node) {
        size_t out_size = 0;
        uint8_t* serialized = glr_serialize_gss_node(node, &out_size);
        
        if (serialized) {
            free(serialized);
        }
        
        free(node);
    }

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif

#ifdef __AFL_FUZZ_TESTCASE_LEN
    }
#endif

    return 0;
}
