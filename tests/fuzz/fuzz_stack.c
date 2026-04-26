/**
 * AFL Fuzzing harness for stack operations
 */

#include <glr/stack.h>
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

        glr_stack_t* stack = glr_stack_create();
        if (!stack) continue;

        glr_stack_node_t* nodes[256] = {NULL};
        size_t node_count = 0;

        size_t pos = 0;
        while (pos + 3 < len && node_count < 256) {
            uint8_t op = buffer[pos++];
            uint32_t state = buffer[pos++];
            uint32_t position = (buffer[pos] << 8) | buffer[pos+1];
            pos += 2;

            switch (op % 5) {
                case 0: {
                    uint8_t parent_idx = (pos < len) ? buffer[pos++] : 0;
                    glr_stack_node_t* parent = (parent_idx < node_count) ? 
                        nodes[parent_idx] : NULL;
                    nodes[node_count++] = glr_stack_push(stack, state, position, parent);
                    break;
                }
                case 1:
                    glr_stack_is_empty(stack);
                    break;
                case 2:
                    glr_stack_get_node_count(stack);
                    break;
                case 3: {
                    uint8_t node_idx = (pos < len) ? buffer[pos++] : 0;
                    if (node_idx < node_count && nodes[node_idx]) {
                        glr_stack_node_get_state(nodes[node_idx]);
                    }
                    break;
                }
                case 4: {
                    uint8_t node_idx = (pos < len) ? buffer[pos++] : 0;
                    if (node_idx < node_count && nodes[node_idx]) {
                        glr_stack_node_get_position(nodes[node_idx]);
                    }
                    break;
                }
            }
        }

        glr_stack_destroy(stack);
    }

    return 0;
}
