#ifndef __STACK_L__
#define __STACK_L__

typedef struct Stack stack_t;

stack_t *stack_create();

int stack_push(stack_t *stack, void *payload);
void *stack_pop(stack_t *stack);

int stack_size(stack_t *stack);

int stack_destroy(stack_t *stack);

#endif