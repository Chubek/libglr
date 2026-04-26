#include <glr/diff.h>
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

    if (len < 2) {
#ifndef __AFL_FUZZ_TESTCASE_LEN
        free(buf);
#endif
        return 0;
    }

    size_t split = len / 2;
    
    const char* old_text = (const char*)buf;
    size_t old_len = split;
    
    const char* new_text = (const char*)(buf + split);
    size_t new_len = len - split;

    glr_edit_region_t region;
    glr_compute_edit_region(old_text, old_len, new_text, new_len, &region);

#ifndef __AFL_FUZZ_TESTCASE_LEN
    free(buf);
#endif

#ifdef __AFL_FUZZ_TESTCASE_LEN
    }
#endif

    return 0;
}
