/**
 * AFL Fuzzing harness for forest operations
 */

#include <glr/forest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_INPUT_SIZE 4096

int main(int argc, char** argv) {
    uint8_t buffer[MAX_INPUT_SIZE];
    size_t len = 0;

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
#endif

    while (__AFL_LOOP(1000)) {
        len = fread(buffer, 1, MAX_INPUT_SIZE, stdin);
        if (len == 0) continue;

        glr_forest_t* forest = glr_forest_create();
        if (!forest) continue;

        glr_forest_node_t* nodes[256] = {NULL};
        size_t node_count = 0;

        size_t pos = 0;
        while (pos + 4 < len && node_count < 256) {
            uint8_t op = buffer[pos++];
            uint32_t symbol_id = buffer[pos++];
            uint32_t start = (buffer[pos] << 8) | buffer[pos+1];
            pos += 2;
            uint32_t end = start + (buffer[pos++] % 100);

            if (op % 2 == 0) {
                char text[64];
                size_t text_len = (pos < len) ? (buffer[pos++] % 63) : 0;
                if (pos + text_len <= len) {
                    memcpy(text, buffer + pos, text_len);
                    text[text_len] = '\0';
                    pos += text_len;
                    
                    nodes[node_count++] = glr_forest_add_terminal(
                        forest, symbol_id, start, end, text);
                }
            } else {
                uint8_t child_count = (pos < len) ? (buffer[pos++] % 5) : 0;
                glr_forest_node_t* children[5];
                
                for (int i = 0; i < child_count; i++) {
                    uint8_t child_idx = (pos < len) ? buffer[pos++] : 0;
                    children[i] = (child_idx < node_count) ? nodes[child_idx] : NULL;
                }
                
                if (child_count > 0 && children[0] != NULL) {
                    nodes[node_count++] = glr_forest_add_nonterminal(
                        forest, symbol_id, start, end, children, child_count);
                }
            }
        }

        glr_forest_destroy(forest);
    }

    return 0;
}
