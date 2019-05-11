#include <stdlib.h>

#include "stack.h"

#define STACK_BASE_SIZE 32

struct stack_struct {
    size_t size;
    size_t cursor;
    addr_t *stk_space;
};

void stk_resize(stack_t *stack)
{
    long factor = 2;
    addr_t *new_space;
    long add_size = stack->size / factor;
    while (add_size > 0)
    {
        new_space = (addr_t *)realloc(stack->stk_space, stack->size + add_size);
        if (new_space != NULL)
        {
            stack->stk_space = new_space;
            stack->size += add_size;
            break;
        }
        factor++;
        add_size = stack->size / factor;
    }
}

stack_t *new_stk()
{
    stack_t *stack = (stack_t *)malloc(sizeof(stack_t));
    stack->stk_space = (addr_t *)malloc(sizeof(addr_t) * STACK_BASE_SIZE);
    stack->cursor = 0;
    stack->size = STACK_BASE_SIZE;
    return stack;
}

int stk_isempty(stack_t *stack)
{
    return stack->cursor <= 0;
}

addr_t stk_top(stack_t *stack)
{
    size_t idx = stack->cursor;
    if (idx > 0)
    {
        idx--;
    }
    return stack->stk_space[idx];
}

addr_t stk_pop(stack_t *stack)
{
    if (stack->cursor > 0)
    {
        stack->cursor--;
    }
    return stack->stk_space[stack->cursor];
}

void stk_push(stack_t *stack, addr_t item)
{
    stack->stk_space[stack->cursor] = item;
    stack->cursor++;
    if(stack->cursor >= stack->size)
    {
        stk_resize(stack);
    }
}

void free_stk(stack_t *stack)
{
    if (stack != NULL)
    {
        if (stack->stk_space != NULL)
        {
            free(stack->stk_space);
        }
        free(stack);
    }
}