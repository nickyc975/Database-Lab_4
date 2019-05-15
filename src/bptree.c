#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "bptree.h"
#include "stack.h"

#define MAX_KEY_NUM 6
#define BASE_LEAF_NUM 4

typedef enum
{
    INNER, LEAF
} node_type;

struct node_struct
{
    addr_t addr;
    node_type type;
    unsigned int key_num;
    int keys[MAX_KEY_NUM];

    union {
        addr_t children[MAX_KEY_NUM + 1];

        struct
        {
            addr_t blk_addrs[MAX_KEY_NUM];
            addr_t next_node;
        };
    };
};

struct key_iter_struct
{
    Buffer *buffer;
    node_t *curr_node;
    unsigned int key_cursor;
};

struct value_blk_iter_struct
{
    Buffer *buffer;
    addr_t init_addr;
    value_blk_t *curr_blk;
    unsigned int value_cursor;
};

int indent = 0;

addr_t _next_addr(bptree_t *bptree);

node_t *_new_node(bptree_t *bptree, addr_t addr, addr_t parent);
node_t *_new_inner(bptree_t *bptree, addr_t addr, addr_t parent);
node_t *_new_leaf(bptree_t *bptree, addr_t addr, addr_t parent);

int _save_node(bptree_t *bptree, node_t *node, int free_after_save);

value_blk_t *_new_value_blk(bptree_t *bptree);
int _save_value_blk(bptree_t *bptree, value_blk_t *value_blk, int free_after_save);

node_t *_query(bptree_t *bptree, int key, stack_t *addr_stk);

void _insert_into_inner(node_t *node, int key, addr_t child);
void _insert_into_leaf(bptree_t *bptree, node_t *node, int key, addr_t value);

int _copy_inner(node_t *src, node_t *dest, unsigned int start);
int _copy_leaf(node_t *src, node_t *dest, unsigned int start);

void _split_inner(bptree_t *bptree, stack_t *addr_stk);
void _split_leaf(bptree_t *bptree, node_t *node, stack_t *addr_stk);

void _print_indent();
void _print_node(bptree_t *bptree, addr_t addr);

void bptree_init(bptree_t *bptree,bptree_meta_t *meta, Buffer *buffer)
{
    bptree->buffer = buffer;
    bptree->last_alloc_addr = meta->last_alloc_addr;
    bptree->root_node = read_node(bptree, meta->root_addr);
    if (bptree->root_node)
    {
        if (meta->leftmost_addr == meta->root_addr)
        {
            bptree->leftmost_node = bptree->root_node;
        }
        else
        {
            bptree->leftmost_node = read_node(bptree, meta->leftmost_addr);
            if (!bptree->leftmost_node)
            {
                perror("Invalid leftmost node address!");
                exit(1);
            }
        }
    }
    else if (meta->root_addr)
    {
        bptree->root_node = _new_leaf(bptree, meta->root_addr, 0);
        bptree->leftmost_node = bptree->root_node;
        _save_node(bptree, bptree->root_node, 0);
    }
    else
    {
        perror("Invalid root node address.");
        exit(1);
    }
}

void bptree_insert(bptree_t *bptree, int key, addr_t value)
{
    stack_t *addr_stk = new_stk();
    node_t *node = _query(bptree, key, addr_stk);
    _insert_into_leaf(bptree, node, key, value);

    if (node->key_num >= MAX_KEY_NUM)
    {
        _split_leaf(bptree, node, addr_stk);
    }
    _save_node(bptree, node, 0);
    if (node != bptree->root_node && node != bptree->leftmost_node)
    {
        free_node(bptree, node);
    }
    free_stk(addr_stk);
}

void bptree_delete(bptree_t *bptree, int key)
{

}

addr_t bptree_query(bptree_t *bptree, int key)
{
    addr_t result = 0;
    node_t *node = _query(bptree, key, NULL);
    if (node != NULL)
    {
        for (int i = 0; i < node->key_num; i++)
        {
            if (node->keys[i] == key)
            {
                result = node->blk_addrs[i];
                break;
            }
        }
        free_node(bptree, node);
    }
    return result;
}

void bptree_print(bptree_t *bptree)
{
    _print_node(bptree, bptree->root_node->addr);

    addr_t next_addr;
    node_t *node = read_node(bptree, bptree->leftmost_node->addr);

    while (node)
    {
        printf("%d, %d: ", node->addr, node->type);
        for (int i = 0; i < node->key_num; i++)
        {
            printf("%d\t", node->keys[i]);
        }
        printf("\n");
        next_addr = node->next_node;
        free_node(bptree, node);
        node = read_node(bptree, next_addr);
    }
}

void bptree_getmeta(bptree_t *bptree, bptree_meta_t *meta)
{
    meta->root_addr = bptree->root_node->addr;
    meta->leftmost_addr = bptree->leftmost_node->addr;
    meta->last_alloc_addr = bptree->last_alloc_addr;
    meta->leaf_num = 0;

    node_t *node;
    unsigned int leaf_num = BASE_LEAF_NUM;
    addr_t node_addr = meta->leftmost_addr;
    meta->leaf_addrs = (addr_t *)malloc(sizeof(addr_t) * leaf_num);
    while(node_addr)
    {
        meta->leaf_addrs[meta->leaf_num] = node_addr;
        meta->leaf_num++;
        if (meta->leaf_num >= leaf_num)
        {
            leaf_num += BASE_LEAF_NUM;
            meta->leaf_addrs = (addr_t *)realloc(meta->leaf_addrs, sizeof(addr_t) * leaf_num);
        }
        node = read_node(bptree, node_addr);
        node_addr = node->next_node;
        free_node(bptree, node);
    }
}

void bptree_free(bptree_t *bptree)
{
    if (bptree->root_node)
    {
        free_node(bptree, bptree->root_node);
    }

    if (bptree->leftmost_node)
    {
        free_node(bptree, bptree->leftmost_node);
    }
}

key_iter_t *new_key_iter(bptree_meta_t *meta, Buffer *buffer)
{
    if (meta->leftmost_addr)
    {
        bptree_t *bptree = &(bptree_t){.buffer = buffer};
        key_iter_t *iter = (key_iter_t *)malloc(sizeof(key_iter_t));

        iter->buffer = buffer;
        iter->curr_node = read_node(bptree, meta->leftmost_addr);
        if (!iter->curr_node)
        {
            printf("Invalid node addr %d\n", meta->leftmost_addr);
            exit(1);
        }
        iter->key_cursor = 0;
        return iter;
    }

    return NULL;
}

inline int has_next_key(key_iter_t *iter)
{
    return iter->key_cursor < iter->curr_node->key_num || iter->curr_node->next_node != 0;
}

inline int next_key(key_iter_t *iter)
{
    int result = 0;
    if (iter->key_cursor < iter->curr_node->key_num)
    {
        result = iter->curr_node->keys[iter->key_cursor];
        iter->key_cursor++;
    }
    else if (iter->curr_node->next_node != 0)
    {
        addr_t next_addr = iter->curr_node->next_node;
        bptree_t *bptree = &(bptree_t){.buffer = iter->buffer};
        free_node(bptree, iter->curr_node);
        iter->curr_node = read_node(bptree, next_addr);
        if (!iter->curr_node)
        {
            printf("Invalid node addr %d\n", next_addr);
            exit(1);
        }
        iter->key_cursor = 0;
        result = iter->curr_node->keys[iter->key_cursor];
        iter->key_cursor++;
    }
    else
    {
        printf("No more keys!\n");
        exit(1);
    }
    return result;
}

inline addr_t curr_blk_addr(key_iter_t *iter)
{
    return iter->curr_node->blk_addrs[iter->key_cursor - 1];
}

void free_key_iter(key_iter_t *iter)
{
    if (iter)
    {
        if (iter->curr_node)
        {
            free_node(&(bptree_t){.buffer = iter->buffer}, iter->curr_node);
        }
        free(iter);
    }
}

inline node_t *read_node(bptree_t *bptree, addr_t addr)
{
    return (node_t *)readBlockFromDisk(addr, bptree->buffer);
}

inline addr_t node_get(node_t *node, int key)
{
    if (node && node->type == LEAF)
    {
        for (int i = 0; i < node->key_num; i++)
        {
            if (node->keys[i] == key)
            {
                return node->blk_addrs[i];
            }
        }
    }
    return 0;
}

inline int node_maxkey(node_t *node)
{
    if (node && node->type == LEAF)
    {
        return node->keys[node->key_num - 1];
    }
    return INT_MAX;
}

inline int node_minkey(node_t *node)
{
    if (node && node->type == LEAF)
    {
        return node->keys[0];
    }
    return INT_MIN;
}

inline addr_t node_next_node(node_t *node)
{
    if (node && node->type == LEAF)
    {
        return node->next_node;
    }
    return 0;
}

inline void free_node(bptree_t *bptree, node_t *node)
{
    freeBlockInBuffer((unsigned char *)node, bptree->buffer);
}

inline value_blk_t *read_value_blk(bptree_t *bptree, addr_t blk_addr)
{
    return (value_blk_t *)readBlockFromDisk(blk_addr, bptree->buffer);
}

value_blk_iter_t *new_value_blk_iter(addr_t addr, Buffer *buffer)
{
    if (addr)
    {
        bptree_t *bptree = &(bptree_t){.buffer = buffer};
        value_blk_iter_t *iter = (value_blk_iter_t *)malloc(sizeof(value_blk_iter_t));

        iter->buffer = buffer;
        iter->init_addr = addr;
        iter->curr_blk = read_value_blk(bptree, addr);
        if (!iter->curr_blk)
        {
            printf("Invalid node addr %d\n", addr);
            exit(1);
        }
        iter->value_cursor = 0;
        return iter;
    }

    return NULL;
}

inline void reset_value_blk_iter(value_blk_iter_t *iter)
{
    bptree_t *bptree = &(bptree_t){.buffer = iter->buffer};
    if (iter->curr_blk)
    {
        free_value_blk(bptree, iter->curr_blk);
    }
    iter->curr_blk = read_value_blk(bptree, iter->init_addr);
    if (!iter->curr_blk)
    {
        printf("Invalid node addr %d\n", iter->init_addr);
        exit(1);
    }
    iter->value_cursor = 0;
}

inline int has_next_value(value_blk_iter_t *iter)
{
    return iter->value_cursor < iter->curr_blk->value_num || iter->curr_blk->next_blk_addr != 0;
}

inline addr_t next_value(value_blk_iter_t *iter)
{
    addr_t result = 0;
    if (iter->value_cursor < iter->curr_blk->value_num)
    {
        result = iter->curr_blk->values[iter->value_cursor];
        iter->value_cursor++;
    }
    else if (iter->curr_blk->next_blk_addr != 0)
    {
        addr_t next_addr = iter->curr_blk->next_blk_addr;
        bptree_t *bptree = &(bptree_t){.buffer = iter->buffer};
        free_value_blk(bptree, iter->curr_blk);
        iter->curr_blk = read_value_blk(bptree, next_addr);
        if (!iter->curr_blk)
        {
            printf("Invalid block addr %d\n", next_addr);
            exit(1);
        }
        iter->value_cursor = 0;
        result = iter->curr_blk->values[iter->value_cursor];
        iter->value_cursor++;
    }
    else
    {
        printf("No more values!\n");
        exit(1);
    }
    return result;
}

void free_value_blk_iter(value_blk_iter_t *iter)
{
    if (iter)
    {
        if (iter->curr_blk)
        {
            free_value_blk(&(bptree_t){.buffer = iter->buffer}, iter->curr_blk);
        }
        free(iter);
    }
}

inline void free_value_blk(bptree_t *bptree, value_blk_t *value_blk)
{
    freeBlockInBuffer((unsigned char *)value_blk, bptree->buffer);
}

inline addr_t _next_addr(bptree_t *bptree)
{
    bptree->last_alloc_addr += sizeof(node_t);
    return bptree->last_alloc_addr;
}

inline node_t *_new_node(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = (node_t *)getNewBlockInBuffer(bptree->buffer);
    node->key_num = 0;
    node->addr = addr ? addr : _next_addr(bptree);
    return node;
}

inline node_t *_new_inner(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = _new_node(bptree, addr, parent);
    node->type = INNER;
    return node;
}

inline node_t *_new_leaf(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = _new_node(bptree, addr, parent);
    node->type = LEAF;
    node->next_node = 0;
    return node;
}

inline int _save_node(bptree_t *bptree, node_t *node, int free_after_save)
{
    int result = writeBlockToDisk((unsigned char *)node, node->addr, bptree->buffer);
    if (free_after_save)
    {
        free_node(bptree, node);
    }
    return result;
}

inline value_blk_t *_new_value_blk(bptree_t *bptree)
{
    value_blk_t *value_blk = (value_blk_t *)getNewBlockInBuffer(bptree->buffer);
    value_blk->value_num = 0;
    value_blk->blk_addr = _next_addr(bptree);
    value_blk->next_blk_addr = 0;
    return value_blk;
}

inline int _save_value_blk(bptree_t *bptree, value_blk_t *value_blk, int free_after_save)
{
    int result = writeBlockToDisk((unsigned char *)value_blk, value_blk->blk_addr, bptree->buffer);
    if (free_after_save)
    {
        free_value_blk(bptree, value_blk);
    }
    return result;
}

node_t *_query(bptree_t *bptree, int key, stack_t *addr_stk)
{
    addr_t next_addr;
    node_t *node = read_node(bptree, bptree->root_node->addr);

    if (addr_stk != NULL)
    {
        stk_push(addr_stk, bptree->root_node->addr);
    }
    while (node && node->type == INNER)
    {
        next_addr = node->children[0];
        for (int i = 0; i < node->key_num; i++)
        {
            if (key >= node->keys[i])
            {
                next_addr = node->children[i + 1];
                continue;
            }
            break;
        }
        if (addr_stk != NULL)
        {
            stk_push(addr_stk, next_addr);
        }
        free_node(bptree, node);
        node = read_node(bptree, next_addr);
    }

    if (node->addr == bptree->leftmost_node->addr)
    {
        free_node(bptree, node);
        node = bptree->leftmost_node;
    }

    return node;
}

void _insert_into_inner(node_t *node, int key, addr_t child)
{
    int temp_key = key, temp_child = child;
    if (node && node->type == INNER)
    {
        for (int i = 0; i < node->key_num; i++)
        {
            if (node->keys[i] > temp_key)
            {
                temp_key ^= node->keys[i];
                temp_child ^= node->children[i + 1];
                node->keys[i] = temp_key ^ node->keys[i];
                node->children[i + 1] = temp_child ^ node->children[i + 1];
                temp_key ^= node->keys[i];
                temp_child ^= node->children[i + 1];
            }
        }
        node->keys[node->key_num] = temp_key;
        node->children[node->key_num + 1] = temp_child;
        node->key_num++;
    }
}

void _insert_into_leaf(bptree_t *bptree, node_t *node, int key, addr_t value)
{
    value_blk_t *value_blk;
    int temp_key = key, temp_blk_addr;
    if (node && node->type == LEAF)
    {
        for (int i = 0; i < node->key_num; i++)
        {
            if (node->keys[i] == temp_key)
            {
                value_blk = read_value_blk(bptree, node->blk_addrs[i]);
                value_blk->values[value_blk->value_num] = value;
                value_blk->value_num++;
                if (value_blk->value_num >= MAX_VALUE_NUM)
                {
                    _save_value_blk(bptree, value_blk, 1);
                    value_blk = _new_value_blk(bptree);
                    value_blk->next_blk_addr = node->blk_addrs[i];
                    node->blk_addrs[i] = value_blk->blk_addr;
                }
                _save_value_blk(bptree, value_blk, 1);
                return;
            }
            else if (node->keys[i] > temp_key)
            {
                break;
            }
        }

        value_blk = _new_value_blk(bptree);

        value_blk->values[value_blk->value_num] = value;
        value_blk->value_num++;
        temp_blk_addr = value_blk->blk_addr;

        _save_value_blk(bptree, value_blk, 1);

        for (int i = 0; i < node->key_num; i++)
        {
            if (node->keys[i] > temp_key)
            {
                temp_key ^= node->keys[i];
                temp_blk_addr ^= node->blk_addrs[i];
                node->keys[i] = temp_key ^ node->keys[i];
                node->blk_addrs[i] = temp_blk_addr ^ node->blk_addrs[i];
                temp_key ^= node->keys[i];
                temp_blk_addr ^= node->blk_addrs[i];
            }
        }

        node->keys[node->key_num] = temp_key;
        node->blk_addrs[node->key_num] = temp_blk_addr;
        node->key_num++;
    }
}

inline int _copy_inner(node_t *src, node_t *dest, unsigned int start)
{
    unsigned int cursor = start;
    while (cursor < src->key_num && dest->key_num < MAX_KEY_NUM)
    {
        dest->keys[dest->key_num] = src->keys[cursor];
        dest->children[dest->key_num] = src->children[cursor];
        dest->key_num++;
        cursor++;
    }
    return cursor - start;
}

inline int _copy_leaf(node_t *src, node_t *dest, unsigned int start)
{
    unsigned int cursor = start;
    while (cursor < src->key_num && dest->key_num < MAX_KEY_NUM)
    {
        dest->keys[dest->key_num] = src->keys[cursor];
        dest->blk_addrs[dest->key_num] = src->blk_addrs[cursor];
        dest->key_num++;
        cursor++;
    }
    return cursor - start;
}

void _split_inner(bptree_t *bptree, stack_t *addr_stk)
{
    node_t *new_node, *parent;
    node_t *node = read_node(bptree, stk_pop(addr_stk));
    unsigned int spliter_idx = (node->key_num + 1) / 2;

    new_node = _new_inner(bptree, 0, 0);
    _copy_inner(node, new_node, spliter_idx);
    new_node->children[new_node->key_num] = node->children[node->key_num];
    node->key_num = spliter_idx;

    unsigned int key_num = 0;
    if (!stk_isempty(addr_stk))
    {
        parent = read_node(bptree, stk_top(addr_stk));
        _insert_into_inner(parent, new_node->keys[0], new_node->addr);
        key_num = parent->key_num;
        _save_node(bptree, parent, 1);
    }
    else
    {
        free_node(bptree, bptree->root_node);
        parent = _new_inner(bptree, node->addr, 0);
        _insert_into_inner(parent, new_node->keys[0], new_node->addr);
        node->addr = _next_addr(bptree);
        parent->children[0] = node->addr;
        bptree->root_node = parent;
        _save_node(bptree, parent, 0);
    }

    _save_node(bptree, new_node, 1);
    _save_node(bptree, node, 1);
    if (key_num >= MAX_KEY_NUM)
    {
        _split_inner(bptree, addr_stk);
    }
}

void _split_leaf(bptree_t *bptree, node_t *node, stack_t *addr_stk)
{
    stk_pop(addr_stk);
    node_t *new_node, *parent;
    unsigned int spliter_idx = (node->key_num + 1) / 2;

    new_node = _new_leaf(bptree, 0, 0);
    new_node->next_node = node->next_node;
    node->next_node = new_node->addr;
    _copy_leaf(node, new_node, spliter_idx);
    node->key_num = spliter_idx;

    unsigned int key_num = 0;
    if (!stk_isempty(addr_stk))
    {
        parent = read_node(bptree, stk_top(addr_stk));
        _insert_into_inner(parent, new_node->keys[0], new_node->addr);
        key_num = parent->key_num;
        _save_node(bptree, parent, 1);
    }
    else
    {
        parent = _new_inner(bptree, node->addr, 0);
        _insert_into_inner(parent, new_node->keys[0], new_node->addr);
        node->addr = _next_addr(bptree);
        parent->children[0] = node->addr;
        bptree->root_node = parent;
        _save_node(bptree, parent, 0);
    }

    _save_node(bptree, new_node, 1);

    if (key_num >= MAX_KEY_NUM)
    {
        _split_inner(bptree, addr_stk);
    }
}

void _print_indent()
{
    for (int i = 0; i < indent * 4; i++)
    {
        printf(" ");
    }
}

void _print_node(bptree_t *bptree, addr_t addr)
{
    addr_t addrs[MAX_KEY_NUM + 1];
    node_t *node = read_node(bptree, addr);
    unsigned int key_num = node->key_num;

    if (node->type == INNER)
    {
        _print_indent();
        printf("%d, %d: ", node->addr, node->type);
        for (int i = 0; i < key_num; i++)
        {
            printf("%d\t", node->keys[i]);
            addrs[i] = node->children[i];
        }
        addrs[key_num] = node->children[key_num];
        free_node(bptree, node);
        printf("\n");

        indent++;
        for (int i = 0; i <= key_num; i++)
        {
            _print_node(bptree, addrs[i]);
        }
        indent--;
    }
    else
    {
        free_node(bptree, node);
    }
}