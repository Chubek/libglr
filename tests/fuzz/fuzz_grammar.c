/**
 * AFL Fuzzing harness for grammar operations
 */

#include <glr/grammar.h>
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

        glr_grammar_t* grammar = glr_grammar_create();
        if (!grammar) continue;

        size_t pos = 0;
        while (pos + 2 < len) {
            uint8_t op = buffer[pos++];
            uint8_t name_len = buffer[pos++];
            
            if (pos + name_len > len) break;
            
            char name[256];
            size_t copy_len = name_len < 255 ? name_len : 255;
            memcpy(name, buffer + pos, copy_len);
            name[copy_len] = '\0';
            pos += name_len;

            switch (op % 4) {
                case 0:
                    glr_grammar_add_terminal(grammar, name);
                    break;
                case 1:
                    glr_grammar_add_nonterminal(grammar, name);
                    break;
                case 2:
                    if (pos + 2 < len) {
                        int nt_id = buffer[pos++];
                        int sym_count = buffer[pos++] % 10;
                        int symbols[10];
                        for (int i = 0; i < sym_count && pos < len; i++) {
                            symbols[i] = buffer[pos++];
                        }
                        glr_grammar_add_production(grammar, nt_id, symbols, sym_count);
                    }
                    break;
                case 3:
                    glr_grammar_finalize(grammar);
                    break;
            }
        }

        glr_grammar_destroy(grammar);
    }

    return 0;
}
