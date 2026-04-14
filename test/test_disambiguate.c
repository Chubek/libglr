#include <glr/disambiguate-hooks.h>
#include <glr/parser.h>
#include <glr/reduction.h>
#include <assert.h>
#include <stdio.h>

int main() {
    // Create mock reductions
    glr_reduction_t reduction1 = { .length = 5, .precedence = 10 };
    glr_reduction_t reduction2 = { .length = 3, .precedence = 5 };
    
    // Create mock context
    glr_disambig_context_t context = {0};
    
    // Test prefer_shorter hook
    glr_disambig_result_t res = glr_disambig_prefer_shorter(&context, &reduction1, &reduction2);
    assert(res == GLR_DISAMBIG_FAILURE);
    
    res = glr_disambig_prefer_shorter(&context, &reduction2, &reduction1);
    assert(res == GLR_DISAMBIG_SUCCESS);
    
    // Test prefer_longer hook
    res = glr_disambig_prefer_longer(&context, &reduction1, &reduction2);
    assert(res == GLR_DISAMBIG_SUCCESS);
    
    res = glr_disambig_prefer_longer(&context, &reduction2, &reduction1);
    assert(res == GLR_DISAMBIG_FAILURE);
    
    // Test prefer_higher_precedence hook
    res = glr_disambig_prefer_higher_precedence(&context, &reduction1, &reduction2);
    assert(res == GLR_DISAMBIG_SUCCESS);
    
    res = glr_disambig_prefer_higher_precedence(&context, &reduction2, &reduction1);
    assert(res == GLR_DISAMBIG_FAILURE);
    
    printf("All disambiguation hook tests passed!\n");
    return 0;
}
