/**
 * AFL Fuzzing harness for reader/lexer operations
 */

#include <glr/reader.h>
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

        glr_reader_t* reader = glr_reader_create_from_string((const char*)buffer, len);
        if (!reader) continue;

        size_t ops = (len > 0) ? buffer[0] : 10;
        for (size_t i = 0; i < ops && i < 100; i++) {
            uint8_t op = (i + 1 < len) ? buffer[i + 1] : 0;
            
            switch (op % 6) {
                case 0:
                    glr_reader_getc(reader);
                    break;
                case 1:
                    glr_reader_ungetc(reader, 'x');
                    break;
                case 2:
                    glr_reader_peek(reader);
                    break;
                case 3:
                    glr_reader_get_position(reader);
                    break;
                case 4:
                    glr_reader_get_line(reader);
                    break;
                case 5:
                    glr_reader_get_column(reader);
                    break;
            }
        }

        glr_reader_destroy(reader);
    }

    return 0;
}
