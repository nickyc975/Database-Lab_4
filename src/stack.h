#ifndef STACK_H
#define STACK_H

#include "../lib/libextmem.h"

typedef struct stack_struct stack_t;

stack_t *new_stk();

int stk_isempty(stack_t *stack);

addr_t stk_top(stack_t *stack);

addr_t stk_pop(stack_t *stack);

void stk_push(stack_t *stack, addr_t item);

void free_stk(stack_t *stack);

#endif