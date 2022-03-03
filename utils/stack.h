#ifndef __STACK_L__
#define __STACK_L__

typedef struct Stack stack_tv2;

stack_tv2 *stack_create();

int stack_push(stack_tv2 *stack, void *payload);

void *stack_peek(stack_tv2 *stack);
void *stack_pop(stack_tv2 *stack);

int stack_size(stack_tv2 *stack);

int stack_destroy(stack_tv2 *stack);

#endif