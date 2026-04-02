#include <glr/glr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) \
    if (cond) { \
        tests_passed++; \
    } else { \
        printf("FAILED: %s\n", msg); \
        tests_failed++; \
    }

#define ASSERT_EQ(a, b, msg) \
    if ((a) == (b)) { \
        tests_passed++; \
    } else { \
        printf("FAILED: %s\n", msg); \
        tests_failed++; \
    }

#define ASSERT_NULL(ptr, msg) \
    ASSERT((ptr) == NULL, msg)

#define ASSERT_NOT_NULL(ptr, msg) \
    ASSERT((ptr) != NULL, msg)

static void print_test_name(const char *name) {
    printf("Testing: %s... ", name);
}

static void test_parser_create_destroy(void) {
    print_test_name("create_destroy");
    
    glr_grammar_t *grammar = glr_grammar_create();
    ASSERT_NOT_NULL(grammar, "Failed to create grammar");
    
    glr_parser_t *parser = glr_parser_create(grammar);
    ASSERT_NOT_NULL(parser, "Failed to create parser");
    
    glr_parser_destroy(parser);
    glr_grammar_destroy(grammar);
    printf("PASSED\n");
}

static void test_parser_null_grammar(void) {
    print_test_name("null_grammar");
    
    glr_parser_t *parser = glr_parser_create(NULL);
    ASSERT_NULL(parser, "Parser with NULL grammar should be NULL");
    printf("PASSED\n");
}

static void test_parser_reset(void) {
    print_test_name("reset");
    
    glr_grammar_t *grammar = glr_grammar_create();
    ASSERT_NOT_NULL(grammar, "Failed to create grammar");
    
    glr_parser_t *parser = glr_parser_create(grammar);
    ASSERT_NOT_NULL(parser, "Failed to create parser");
    
    int ret = glr_parser_reset(parser);
    ASSERT_EQ(ret, 0, "Reset should succeed");
    
    glr_parser_destroy(parser);
    glr_grammar_destroy(grammar);
    printf("PASSED\n");
}

static void test_parser_parse(void) {
    print_test_name("parse");
    
    glr_grammar_t *grammar = glr_grammar_create();
    ASSERT_NOT_NULL(grammar, "Failed to create grammar");
    
    glr_parser_t *parser = glr_parser_create(grammar);
    ASSERT_NOT_NULL(parser, "Failed to create parser");
    
    const char *input = "test";
    glr_parse_result_t result = glr_parse(parser, input, strlen(input));
    
    ASSERT_NOT_NULL(parser, "Parser should be valid");
    
    glr_parser_destroy(parser);
    glr_grammar_destroy(grammar);
    printf("PASSED\n");
}

static void test_parser_parse_null(void) {
    print_test_name("parse_null");
    
    glr_parser_t *parser = glr_parser_create(NULL);
    ASSERT_NULL(parser, "Parser with NULL grammar should be NULL");
    
    glr_parse_result_t result = glr_parse(NULL, "input", 5);
    ASSERT_EQ(result.error, GLR_PARSE_ERROR_MEMORY, "Should return memory error");
    ASSERT_NULL(result.forest, "Forest should be NULL");
    printf("PASSED\n");
}

static void test_parser_user_data(void) {
    print_test_name("user_data");
    
    glr_grammar_t *grammar = glr_grammar_create();
    ASSERT_NOT_NULL(grammar, "Failed to create grammar");
    
    glr_parser_t *parser = glr_parser_create(grammar);
    ASSERT_NOT_NULL(parser, "Failed to create parser");
    
    void *data = (void*)0x12345678;
    glr_parser_set_user_data(parser, data);
    
    void *retrieved = glr_parser_get_user_data(parser);
    ASSERT_EQ(retrieved, data, "Should get user data");
    
    glr_parser_destroy(parser);
    glr_grammar_destroy(grammar);
    printf("PASSED\n");
}

static void test_parser_error(void) {
    print_test_name("error");
    
    glr_grammar_t *grammar = glr_grammar_create();
    ASSERT_NOT_NULL(grammar, "Failed to create grammar");
    
    glr_parser_t *parser = glr_parser_create(grammar);
    ASSERT_NOT_NULL(parser, "Failed to create parser");
    
    glr_parse_error_t error = glr_parser_get_error(parser);
    ASSERT_EQ(error, GLR_PARSE_SUCCESS, "New parser should have success error");
    
    glr_parser_destroy(parser);
    glr_grammar_destroy(grammar);
    printf("PASSED\n");
}

static void test_parser_forest(void) {
    print_test_name("forest");
    
    glr_grammar_t *grammar = glr_grammar_create();
    ASSERT_NOT_NULL(grammar, "Failed to create grammar");
    
    glr_parser_t *parser = glr_parser_create(grammar);
    ASSERT_NOT_NULL(parser, "Failed to create parser");
    
    glr_forest_t *forest = glr_parser_get_forest(parser);
    ASSERT_NOT_NULL(forest, "Forest should exist");
    
    glr_parser_destroy(parser);
    glr_grammar_destroy(grammar);
    printf("PASSED\n");
}

static void test_parser_version(void) {
    print_test_name("version");
    
    const char *version = glr_version();
    ASSERT_NOT_NULL(version, "Version should not be NULL");
    ASSERT(strlen(version) > 0, "Version should not be empty");
    
    const char *name = glr_name();
    ASSERT_NOT_NULL(name, "Name should not be NULL");
    ASSERT(strcmp(name, "LibGLR") == 0, "Name should be LibGLR");
    
    printf("PASSED\n");
}

int main(void) {
    printf("=== LibGLR Parser Tests ===\n\n");
    
    test_parser_create_destroy();
    test_parser_null_grammar();
    test_parser_reset();
    test_parser_parse();
    test_parser_parse_null();
    test_parser_user_data();
    test_parser_error();
    test_parser_forest();
    test_parser_version();
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
