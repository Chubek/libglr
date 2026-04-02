#include <glr/stack.h>
#include <stdlib.h>
#include <string.h>

struct glr_stack_node {
    void *state;
    size_t height;
    struct glr_stack_node *parent;
    struct glr_stack_node *child;
    struct glr_stack_node *sibling;
};

glr_stack_t *glr_stack_create(void) {
    glr_stack_t *stack = calloc(1, sizeof(glr_stack_t));
    if (stack == NULL) {
        return NULL;
    }
    
    stack->capacity = 16;
    stack->states = calloc(stack->capacity, sizeof(void *));
    if (stack->states == NULL) {
        free(stack);
        return NULL;
    }
    
    stack->height = 0;
    stack->root = NULL;
    
    return stack;
}

void glr_stack_destroy(glr_stack_t *stack) {
    if (stack == NULL) {
        return;
    }
    
    free(stack->states);
    free(stack);
}

glr_stack_t *glr_stack_fork(glr_stack_t *stack, size_t height) {
    if (stack == NULL || height > stack->height) {
        return NULL;
    }
    
    /* Create new stack */
    glr_stack_t *fork = calloc(1, sizeof(glr_stack_t));
    if (fork == NULL) {
        return NULL;
    }
    
    fork->capacity = stack->capacity;
    fork->states = malloc(fork->capacity * sizeof(void *));
    if (fork->states == NULL) {
        free(fork);
        return NULL;
    }
    
    /* Copy states up to fork height */
    fork->height = height;
    memcpy(fork->states, stack->states, height * sizeof(void *));
    
    return fork;
}

int glr_stack_push(glr_stack_t *stack, void *state) {
    if (stack == NULL || stack->height >= stack->capacity) {
        /* Expand capacity if needed */
        size_t new_capacity = stack->capacity * 2;
        void **new_states = realloc(stack->states, new_capacity * sizeof(void *));
        if (new_states == NULL) {
            return -1;
        }
        
        stack->capacity = new_capacity;
        stack->states = new_states;
    }
    
    stack->states[stack->height] = state;
    stack->height++;
    
    return 0;
}

void *glr_stack_pop(glr_stack_t *stack) {
    if (stack == NULL || stack->height == 0) {
        return NULL;
    }
    
    stack->height--;
    void *state = stack->states[stack->height];
    stack->states[stack->height] = NULL;
    
    return state;
}

void *glr_stack_peek(glr_stack_t *stack) {
    if (stack == NULL || stack->height == 0) {
        return NULL;
    }
    
    return stack->states[stack->height - 1];
}

void *glr_stack_get(glr_stack_t *stack, size_t height) {
    if (stack == NULL || height >= stack->height) {
        return NULL;
    }
    
    return stack->states[height];
}

size_t glr_stack_height(glr_stack_t *stack) {
    return stack != NULL ? stack->height : 0;
}
