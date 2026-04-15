#ifndef GLR_READER_H
#define GLR_READER_H

#include <glr/lexer-hooks.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @file reader.h
   * @brief UTF-16 reader with pluggable lexer hooks.
   */

  typedef struct glr_reader glr_reader_t;

  typedef enum
  {
    GLR_READER_ENCODING_UTF16_LE,
    GLR_READER_ENCODING_UTF16_BE,
    GLR_READER_ENCODING_UTF16_AUTO
  } glr_reader_encoding_t;

  typedef enum
  {
    GLR_READER_STATUS_OK,
    GLR_READER_STATUS_EOF,
    GLR_READER_STATUS_INVALID_ARGUMENT,
    GLR_READER_STATUS_INVALID_ENCODING,
    GLR_READER_STATUS_INVALID_SEQUENCE,
    GLR_READER_STATUS_NO_MEMORY
  } glr_reader_status_t;

  typedef struct
  {
    char *terminal_name;
    uint32_t codepoint;
    size_t byte_offset;
    size_t bytes_consumed;
    bool from_hook;
  } glr_reader_token_t;

  glr_reader_t *glr_reader_create (void);
  void glr_reader_destroy (glr_reader_t *reader);
  int glr_reader_set_input (glr_reader_t *reader, const void *input,
                            size_t input_length);
  void glr_reader_reset (glr_reader_t *reader);

  void glr_reader_set_encoding (glr_reader_t *reader,
                                glr_reader_encoding_t encoding);
  glr_reader_encoding_t glr_reader_get_encoding (const glr_reader_t *reader);

  int glr_reader_set_lexer_hooks (glr_reader_t *reader, glr_lexer_hooks_t *hooks);
  glr_lexer_hooks_t *glr_reader_get_lexer_hooks (const glr_reader_t *reader);

  glr_reader_status_t glr_reader_next (glr_reader_t *reader,
                                       glr_reader_token_t *token);
  void glr_reader_token_clear (glr_reader_token_t *token);
  const char *glr_reader_status_string (glr_reader_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* GLR_READER_H */
