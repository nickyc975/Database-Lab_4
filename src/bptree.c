#include <stdlib.h>

#include "bptree.h"

#define INNER 0
#define LEAF 1
#define OVERFLOW 2
#define MAX_VALID_LEN 7
#define ROOT_ADDR 0x696478

#define STACK_BASE_SIZE 32

typedef struct stack_struct {
    size_t size;
    size_t cursor;
    addr_t *stk_space;
} stack_t;

typedef struct node_struct
{
    unsigned char type;
    unsigned char overflowed;
    unsigned short valid_length;
    
    int keys[MAX_VALID_LEN];
    union
    {
        addr_t addrs[MAX_VALID_LEN + 1];
        
        struct
        {
            int values[MAX_VALID_LEN];
            addr_t next_node;
        };
    };
} node_t;

struct bptree_struct 
{
    node_t *root;
    Buffer *buffer;
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

node_t *_bptree_query(bptree_t *bptree, int key, stack_t *addr_stk)
{
    addr_t next_addr;
    node_t *node = bptree->root;

    while(node && node->type == INNER)
    {
        next_addr = node->addrs[0];
        for (int i = 0; i < node->valid_length; i++)
        {
            if (key > node->keys[i])
            {
                next_addr = node->addrs[i + 1];
                continue;
            }
            break;
        }
        stk_push(addr_stk, next_addr);
        freeBlockInBuffer((unsigned char *)node, bptree->buffer);
        node = (node_t *)readBlockFromDisk(next_addr, bptree->buffer);
    }

    return node;
}

void _insert_into_node(node_t *node, int key, int value)
{
    int temp_key = key, temp_value = value;
    if (node && node->type == LEAF)
    {
        for (int i = 0; i < node->valid_length; i++)
        {
            if (node->keys[i] > temp_key) {
                temp_key += node->keys[i];
                temp_value += node->values[i];
                node->keys[i] = temp_key - node->keys[i];
                node->values[i] = temp_value - node->values[i];
                temp_key -= node->keys[i];
                temp_value -= node->values[i];
            }
        }
        node->keys[node->valid_length] = temp_key;
        node->values[node->valid_length] = temp_value;
        node->valid_length++;
    }
}

void _bptree_split(node_t *node, stack_t *addr_stk)
{

}

void bptree_init(bptree_t *bptree, Buffer *buffer)
{
    bptree->buffer = buffer;
    bptree->root = (node_t *)readBlockFromDisk(ROOT_ADDR, bptree->buffer);
    if (bptree->root == NULL)
    {
        bptree->root = (node_t *)getNewBlockInBuffer(bptree->buffer);
    }
}

void bptree_insert(bptree_t *bptree, int key, int value)
{
    stack_t *addr_stk = new_stk();
    node_t *node = _bptree_query(bptree, key, addr_stk);
    addr_t node_addr = stk_top(addr_stk);
    
    _insert_into_node(node, key, value);
    if (node->valid_length >= MAX_VALID_LEN)
    {
        _bptree_split(node, addr_stk);
    }
    writeBlockToDisk((unsigned char *)node, node_addr, bptree->buffer);
    freeBlockInBuffer((unsigned char *)node, bptree->buffer);
    free_stk(addr_stk);
}

void bptree_delete(bptree_t *bptree, int key)
{

}

void bptree_query(bptree_t *bptree, int key, addr_t base_addr)
{

}

void bptree_free(bptree_t *bptree)
{

}