#include <stdlib.h>

#include "stack.h"

typedef struct LinkedList {
	void *payload;

	struct LinkedList *next;
} stack_ll_t;

struct Stack {
	stack_ll_t *stack_head;

	int size;
};

stack_t *stack_create() {
	stack_t *new_stack = malloc(sizeof(stack_t));

	new_stack->stack_head = NULL;
	new_stack->size = 0;

	return new_stack;
}

int stack_push(stack_t *stack, void *payload) {
	stack_ll_t *new_head = malloc(sizeof(stack_ll_t));

	new_head->payload = payload;
	new_head->next = NULL;

	if (stack->stack_head) {
		new_head->next = stack->stack_head;
	}

	stack->stack_head = new_head;
	stack->size++;

	return 0;
}

void *stack_pop(stack_t *stack) {
	stack_ll_t *pull_head = stack->stack_head;

	if (!pull_head)
		return NULL;

	// update head
	stack->stack_head = pull_head->next;

	void *pull_head_payload = pull_head->payload;

	free(pull_head);

	stack->size--;
	return pull_head_payload;
}

int stack_size(stack_t *stack) {
	return stack->size;
}

int stack_destroy(stack_t *stack) {
	while (stack->stack_head) {
		stack_ll_t *next = stack->stack_head->next;

		free(stack->stack_head);

		stack->stack_head = next;
	}

	free(stack);

	return 0;
}