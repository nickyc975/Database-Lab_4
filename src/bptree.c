#include <stdio.h>
#include <stdlib.h>

#include "bptree.h"
#include "stack.h"

#define INNER 0
#define LEAF 1

#define MAX_KEY_NUM 6

#define ROOT_ADDR 0x696478

struct node_struct
{
    addr_t addr;

    unsigned int type;
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

int indent = 0;

addr_t _next_addr(bptree_t *bptree);

node_t *_new_node(bptree_t *bptree, addr_t addr, addr_t parent);
node_t *_new_inner(bptree_t *bptree, addr_t addr, addr_t parent);
node_t *_new_leaf(bptree_t *bptree, addr_t addr, addr_t parent);

node_t *_read_node(bptree_t *bptree, addr_t addr);
int _save_node(bptree_t *bptree, node_t *node, int free_after_save);
void _free_node(bptree_t *bptree, node_t *node);

data_blk_t *_new_data_blk(bptree_t *bptree);
int _save_data_blk(bptree_t *bptree, data_blk_t *data_blk, int free_after_save);

node_t *_query(bptree_t *bptree, int key, stack_t *addr_stk);

void _insert_into_inner(node_t *node, int key, addr_t child);
void _insert_into_leaf(bptree_t *bptree, node_t *node, int key, addr_t value);

int _copy_inner(node_t *src, node_t *dest, unsigned int start);
int _copy_leaf(node_t *src, node_t *dest, unsigned int start);

void _split_inner(bptree_t *bptree, stack_t *addr_stk);
void _split_leaf(bptree_t *bptree, node_t *node, stack_t *addr_stk);

void _print_indent();
void _print_node(bptree_t *bptree, addr_t addr);

void bptree_init(bptree_t *bptree, Buffer *buffer)
{
    bptree->buffer = buffer;
    bptree->root = _read_node(bptree, ROOT_ADDR);
    if (bptree->root == NULL)
    {
        bptree->root = _new_leaf(bptree, ROOT_ADDR, 0);
        _save_node(bptree, bptree->root, 0);
    }
    bptree->curr_addr = ROOT_ADDR;
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
    if (node->addr != ROOT_ADDR)
    {
        _free_node(bptree, node);
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
        _free_node(bptree, node);
    }
    return result;
}

void bptree_print(bptree_t *bptree)
{
    addr_t next_addr;
    node_t *node = bptree->root;

    _print_node(bptree, bptree->root->addr);

    while (node && node->type == INNER)
    {
        next_addr = node->children[0];
        _free_node(bptree, node);
        node = _read_node(bptree, next_addr);
    }

    while (node)
    {
        printf("%d, %d: ", node->addr, node->type);
        for (int i = 0; i < node->key_num; i++)
        {
            printf("%d\t", node->keys[i]);
        }
        printf("\n");
        next_addr = node->next_node;
        _free_node(bptree, node);
        node = _read_node(bptree, next_addr);
    }
}

void bptree_free(bptree_t *bptree)
{

}

data_blk_t *read_data_blk(bptree_t *bptree, addr_t blk_addr)
{
    return (data_blk_t *)readBlockFromDisk(blk_addr, bptree->buffer);
}

void free_data_blk(bptree_t *bptree, data_blk_t *data_blk)
{
    freeBlockInBuffer((unsigned char *)data_blk, bptree->buffer);
}

addr_t _next_addr(bptree_t *bptree)
{
    bptree->curr_addr += sizeof(node_t);
    return bptree->curr_addr;
}

node_t *_new_node(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = (node_t *)getNewBlockInBuffer(bptree->buffer);
    node->key_num = 0;
    node->addr = addr ? addr : _next_addr(bptree);
    return node;
}

node_t *_new_inner(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = _new_node(bptree, addr, parent);
    node->type = (unsigned short)INNER;
    return node;
}

node_t *_new_leaf(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = _new_node(bptree, addr, parent);
    node->type = (unsigned short)LEAF;
    return node;
}

node_t *_read_node(bptree_t *bptree, addr_t addr)
{
    return (node_t *)readBlockFromDisk(addr, bptree->buffer);
}

int _save_node(bptree_t *bptree, node_t *node, int free_after_save)
{
    int result = writeBlockToDisk((unsigned char *)node, node->addr, bptree->buffer);
    if (free_after_save)
    {
        _free_node(bptree, node);
    }
    return result;
}

void _free_node(bptree_t *bptree, node_t *node)
{
    freeBlockInBuffer((unsigned char *)node, bptree->buffer);
}

data_blk_t *_new_data_blk(bptree_t *bptree)
{
    data_blk_t *data_blk = (data_blk_t *)getNewBlockInBuffer(bptree->buffer);
    data_blk->value_num = 0;
    data_blk->blk_addr = _next_addr(bptree);
    data_blk->next_blk_addr = 0;
    return data_blk;
}

int _save_data_blk(bptree_t *bptree, data_blk_t *data_blk, int free_after_save)
{
    int result = writeBlockToDisk((unsigned char *)data_blk, data_blk->blk_addr, bptree->buffer);
    if (free_after_save)
    {
        free_data_blk(bptree, data_blk);
    }
    return result;
}

node_t *_query(bptree_t *bptree, int key, stack_t *addr_stk)
{
    addr_t next_addr;
    node_t *node = bptree->root;

    if (addr_stk != NULL)
    {
        stk_push(addr_stk, ROOT_ADDR);
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
        _free_node(bptree, node);
        node = _read_node(bptree, next_addr);
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
    data_blk_t *data_blk;
    int temp_key = key, temp_blk_addr;
    if (node && node->type == LEAF)
    {
        for (int i = 0; i < node->key_num; i++)
        {
            if (node->keys[i] == temp_key)
            {
                data_blk = read_data_blk(bptree, node->blk_addrs[i]);
                data_blk->values[data_blk->value_num] = value;
                data_blk->value_num++;
                if (data_blk->value_num >= MAX_VALUE_NUM)
                {
                    _save_data_blk(bptree, data_blk, 1);
                    data_blk = _new_data_blk(bptree);
                    data_blk->next_blk_addr = node->blk_addrs[i];
                    node->blk_addrs[i] = data_blk->blk_addr;
                }
                _save_data_blk(bptree, data_blk, 1);
                return;
            }
            else if (node->keys[i] > temp_key)
            {
                break;
            }
        }

        data_blk = _new_data_blk(bptree);

        data_blk->values[data_blk->value_num] = value;
        data_blk->value_num++;
        temp_blk_addr = data_blk->blk_addr;

        _save_data_blk(bptree, data_blk, 1);

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

int _copy_inner(node_t *src, node_t *dest, unsigned int start)
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

int _copy_leaf(node_t *src, node_t *dest, unsigned int start)
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
    node_t *node = _read_node(bptree, stk_pop(addr_stk));
    unsigned int spliter_idx = (node->key_num + 1) / 2;

    new_node = _new_inner(bptree, 0, 0);
    _copy_inner(node, new_node, spliter_idx);
    new_node->children[new_node->key_num] = node->children[node->key_num];
    node->key_num = spliter_idx;

    if (!stk_isempty(addr_stk))
    {
        parent = _read_node(bptree, stk_top(addr_stk));
    }
    else
    {
        parent = _new_inner(bptree, ROOT_ADDR, 0);
        node->addr = _next_addr(bptree);
        parent->children[0] = node->addr;
        bptree->root = parent;
    }
    _insert_into_inner(parent, new_node->keys[0], new_node->addr);
    unsigned int key_num = parent->key_num;
    _save_node(bptree, new_node, 1);
    _save_node(bptree, parent, 0);
    _save_node(bptree, node, 1);

    if (parent->addr != ROOT_ADDR)
    {
        _free_node(bptree, parent);
    }

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

    if (!stk_isempty(addr_stk))
    {
        parent = _read_node(bptree, stk_top(addr_stk));
    }
    else
    {
        parent = _new_inner(bptree, ROOT_ADDR, 0);
        node->addr = _next_addr(bptree);
        parent->children[0] = node->addr;
        bptree->root = parent;
    }
    _insert_into_inner(parent, new_node->keys[0], new_node->addr);
    unsigned int key_num = parent->key_num;
    _save_node(bptree, new_node, 1);
    _save_node(bptree, parent, 0);

    if (parent->addr != ROOT_ADDR)
    {
        _free_node(bptree, parent);
    }

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
    node_t *node = _read_node(bptree, addr);
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
        _free_node(bptree, node);
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
        _free_node(bptree, node);
    }
}