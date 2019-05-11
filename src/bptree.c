#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "bptree.h"

#define INNER 0
#define LEAF 1
#define MAX_KEY_NUM 6
#define ROOT_ADDR 0x696478
#define MAX_VALUE_NUM 13

struct node_struct
{
    addr_t addr;
    addr_t parent;

    unsigned short type;
    unsigned short key_num;

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

typedef struct data_blk_struct
{
    int value_num;
    addr_t blk_addr;
    addr_t next_blk_addr;
    addr_t values[MAX_VALUE_NUM];
} data_blk_t;

int indent = 0;

void _print_indent()
{
    for (int i = 0; i < indent * 4; i++)
    {
        printf(" ");
    }
}

addr_t _bptree_next_addr(bptree_t *bptree)
{
    bptree->curr_addr += sizeof(node_t);
    return bptree->curr_addr;
}

node_t *_new_node(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = (node_t *)getNewBlockInBuffer(bptree->buffer);
    node->key_num = 0;
    node->addr = addr ? addr : _bptree_next_addr(bptree);
    node->parent = parent;
    return node;
}

node_t *_new_inner_node(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = _new_node(bptree, addr, parent);
    node->type = (unsigned short)INNER;
    return node;
}

node_t *_new_leaf_node(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = _new_node(bptree, addr, parent);
    node->type = (unsigned short)LEAF;
    return node;
}

node_t *_read_node(bptree_t *bptree, addr_t addr)
{
    return (node_t *)readBlockFromDisk(addr, bptree->buffer);
}

void _free_node(bptree_t *bptree, node_t *node)
{
    freeBlockInBuffer((unsigned char *)node, bptree->buffer);
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

data_blk_t *_new_data_blk(bptree_t *bptree)
{
    data_blk_t *data_blk = (data_blk_t *)getNewBlockInBuffer(bptree->buffer);
    data_blk->value_num = 0;
    data_blk->blk_addr = _bptree_next_addr(bptree);
    data_blk->next_blk_addr = 0;
    return data_blk;
}

data_blk_t *_read_data_blk(bptree_t *bptree, addr_t blk_addr)
{
    return (data_blk_t *)readBlockFromDisk(blk_addr, bptree->buffer);
}

void _free_data_blk(bptree_t *bptree, data_blk_t *data_blk)
{
    freeBlockInBuffer((unsigned char *)data_blk, bptree->buffer);
}

int _save_data_blk(bptree_t *bptree, data_blk_t *data_blk, int free_after_save)
{
    int result = writeBlockToDisk((unsigned char *)data_blk, data_blk->blk_addr, bptree->buffer);
    if (free_after_save)
    {
        _free_data_blk(bptree, data_blk);
    }
    return result;
}

node_t *_bptree_query(bptree_t *bptree, int key)
{
    addr_t next_addr;
    node_t *node = bptree->root;

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
                data_blk = _read_data_blk(bptree, node->blk_addrs[i]);
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

int _copy_inner_node(node_t *src, node_t *dest, unsigned int start)
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

int _copy_leaf_node(node_t *src, node_t *dest, unsigned int start)
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

void _bptree_split_inner(bptree_t *bptree, addr_t node_addr)
{
    node_t *new_node, *parent;
    node_t *node = _read_node(bptree, node_addr);
    unsigned int spliter_idx = (node->key_num + 1) / 2;

    new_node = _new_inner_node(bptree, 0, 0);
    _copy_inner_node(node, new_node, spliter_idx);
    new_node->children[new_node->key_num] = node->children[node->key_num];
    node->key_num = spliter_idx;

    assert(new_node->keys[0] == node->keys[spliter_idx]);

    if (node->parent)
    {
        parent = _read_node(bptree, node->parent);
    }
    else
    {
        parent = _new_inner_node(bptree, ROOT_ADDR, 0);
        node->addr = _bptree_next_addr(bptree);
        parent->children[0] = node->addr;
        node->parent = parent->addr;
        bptree->root = parent;
    }
    _insert_into_inner(parent, new_node->keys[0], new_node->addr);

    new_node->parent = node->parent;
    _save_node(bptree, new_node, 1);

    addr_t parent_addr = node->parent;
    _save_node(bptree, node, 1);

    unsigned int key_num = parent->key_num;
    _save_node(bptree, parent, 0);

    if (parent->addr != ROOT_ADDR)
    {
        _free_node(bptree, parent);
    }

    if (key_num >= MAX_KEY_NUM)
    {
        _bptree_split_inner(bptree, parent_addr);
    }
}

void _bptree_split_leaf(bptree_t *bptree, node_t *node)
{
    node_t *new_node, *parent;
    unsigned int spliter_idx = (node->key_num + 1) / 2;

    new_node = _new_leaf_node(bptree, 0, 0);
    new_node->next_node = node->next_node;
    node->next_node = new_node->addr;
    _copy_leaf_node(node, new_node, spliter_idx);
    node->key_num = spliter_idx;

    assert(new_node->keys[0] == node->keys[spliter_idx]);

    if (node->parent)
    {
        parent = _read_node(bptree, node->parent);
    }
    else
    {
        parent = _new_inner_node(bptree, ROOT_ADDR, 0);
        node->addr = _bptree_next_addr(bptree);
        parent->children[0] = node->addr;
        node->parent = parent->addr;
        bptree->root = parent;
    }
    _insert_into_inner(parent, new_node->keys[0], new_node->addr);

    new_node->parent = node->parent;
    _save_node(bptree, new_node, 1);

    unsigned int key_num = parent->key_num;
    _save_node(bptree, parent, 0);

    if (parent->addr != ROOT_ADDR)
    {
        _free_node(bptree, parent);
    }

    if (key_num >= MAX_KEY_NUM)
    {
        _bptree_split_inner(bptree, node->parent);
    }
}

void bptree_init(bptree_t *bptree, Buffer *buffer)
{
    bptree->buffer = buffer;
    bptree->root = _read_node(bptree, ROOT_ADDR);
    if (bptree->root == NULL)
    {
        bptree->root = _new_leaf_node(bptree, ROOT_ADDR, 0);
        _save_node(bptree, bptree->root, 0);
    }
    bptree->curr_addr = ROOT_ADDR;
}

void bptree_insert(bptree_t *bptree, int key, addr_t addr)
{
    node_t *node = _bptree_query(bptree, key);
    _insert_into_leaf(bptree, node, key, addr);

    if (node->key_num >= MAX_KEY_NUM)
    {
        _bptree_split_leaf(bptree, node);
    }
    _save_node(bptree, node, 0);
    if (node->addr != ROOT_ADDR)
    {
        _free_node(bptree, node);
    }
}

void bptree_delete(bptree_t *bptree, int key)
{

}

void bptree_query(bptree_t *bptree, int key, addr_t base_addr)
{

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