#include <stdlib.h>

#include "bptree.h"

#define INNER 0
#define LEAF 1
#define OVERFLOW 2
#define MAX_KEY_NUM 6
#define ROOT_ADDR 0x696478

struct node_struct
{
    addr_t addr;
    addr_t parent;

    unsigned char type;
    unsigned char overflowed;
    unsigned short key_num;

    int keys[MAX_KEY_NUM];
    union
    {
        addr_t children[MAX_KEY_NUM + 1];

        struct
        {
            addr_t values[MAX_KEY_NUM];
            addr_t next_node;
        };
    };
};

addr_t _bptree_next_addr(bptree_t *bptree)
{
    bptree->curr_addr += sizeof(node_t);
    return bptree->curr_addr;
}

node_t *_new_node(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = (node_t *)getNewBlockInBuffer(bptree->buffer);
    node->overflowed = 0;
    node->key_num = 0;
    node->addr = addr ? addr : _bptree_next_addr(bptree);
    node->parent = parent;
    return node;
}

node_t *_new_inner_node(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = _new_node(bptree, addr, parent);
    node->type = INNER;
    return node;
}

node_t *_new_leaf_node(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = _new_node(bptree, addr, parent);
    node->type = LEAF;
    return node;
}

node_t *_new_overflow_node(bptree_t *bptree, addr_t addr, addr_t parent)
{
    node_t *node = _new_node(bptree, addr, parent);
    node->type = OVERFLOW;
    return node;
}

node_t *_read_node(bptree_t *bptree, addr_t addr)
{
    return (node_t *)readBlockFromDisk(addr, bptree->buffer);
}

int _save_node(bptree_t *bptree, node_t *node)
{
    return writeBlockToDisk((unsigned char *)node, node->addr, bptree->buffer);
}

void _free_node(bptree_t *bptree, node_t *node)
{
    freeBlockInBuffer((unsigned char *)node, bptree->buffer);
}

node_t *_bptree_query(bptree_t *bptree, int key)
{
    addr_t next_addr;
    node_t *node = bptree->root;

    while(node && node->type == INNER)
    {
        next_addr = node->children[0];
        for (int i = 0; i < node->key_num; i++)
        {
            if (key > node->keys[i])
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
            if (node->keys[i] > temp_key) {
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

void _insert_into_leaf(node_t *node, int key, addr_t value)
{
    int temp_key = key, temp_value = value;
    if (node && node->type == LEAF)
    {
        for (int i = 0; i < node->key_num; i++)
        {
            if (node->keys[i] > temp_key) {
                temp_key ^= node->keys[i];
                temp_value ^= node->values[i];
                node->keys[i] = temp_key ^ node->keys[i];
                node->values[i] = temp_value ^ node->values[i];
                temp_key ^= node->keys[i];
                temp_value ^= node->values[i];
            }
        }
        node->keys[node->key_num] = temp_key;
        node->values[node->key_num] = temp_value;
        node->key_num++;
    }
}

void _copy_node(node_t *src, node_t *dest, unsigned int start)
{
    unsigned int cursor = start;
    while(cursor < src->key_num && dest->key_num < MAX_KEY_NUM)
    {
        dest->keys[dest->key_num] = src->keys[cursor];
        dest->children[dest->key_num] = src->children[cursor];
        dest->key_num++;
        cursor++;
    }
}

void _bptree_split_inner(bptree_t *bptree, node_t *node)
{
    node_t *new_node, *parent;
    unsigned int spliter_idx = (node->key_num + 1) / 2;

    new_node = _new_inner_node(bptree, 0, 0);
    _copy_node(node, new_node, spliter_idx);
    new_node->children[new_node->key_num] = node->children[node->key_num];
    node->key_num = spliter_idx;
    _save_node(bptree, new_node);
    _free_node(bptree, new_node);

    if (node->parent)
    {
        parent = _read_node(bptree, node->parent);
        _insert_into_inner(parent, node->keys[spliter_idx], new_node->addr);
        if (parent->key_num >= MAX_KEY_NUM)
        {
            _bptree_split_inner(bptree, parent);
        }
        _save_node(bptree, parent);
        _free_node(bptree, parent);
    }
    else
    {
        parent = _new_inner_node(bptree, ROOT_ADDR, 0);
        _insert_into_inner(parent, node->keys[spliter_idx], new_node->addr);
        node->addr = _bptree_next_addr(bptree);
        parent->children[0] = node->addr;
        node->parent = parent->addr;
        bptree->root = parent;
        _save_node(bptree, parent);
    }
}

void _bptree_split_leaf(bptree_t *bptree, node_t *node)
{
    node_t *new_node, *parent;
    unsigned int spliter_idx = (node->key_num + 1) / 2;

    while(spliter_idx < node->key_num && 
        node->keys[spliter_idx - 1] == node->keys[spliter_idx])
    {
        spliter_idx++;
    }
    
    if (spliter_idx >= node->key_num)
    {
        spliter_idx = (node->key_num + 1) / 2;
        if (node->overflowed)
        {
            new_node = _read_node(bptree, node->next_node);
            _copy_node(node, new_node, spliter_idx);

            if (new_node->key_num >= MAX_KEY_NUM)
            {
                _save_node(bptree, new_node);
                _free_node(bptree, new_node);
                new_node = _new_overflow_node(bptree, 0, 0);
                new_node->next_node = node->next_node;
                node->next_node = new_node->addr;
            }
        }
        else
        {
            node->overflowed = 1;
            new_node = _new_overflow_node(bptree, 0, 0);
            new_node->next_node = node->next_node;
            node->next_node = new_node->addr;
        }
    }
    else
    {
        new_node = _new_leaf_node(bptree, 0, 0);
        new_node->next_node = node->next_node;
        node->next_node = new_node->addr;

        if (node->parent)
        {
            parent = _read_node(bptree, node->parent);
            _insert_into_inner(parent, node->keys[spliter_idx], node->next_node);
            if (parent->key_num >= MAX_KEY_NUM)
            {
                _bptree_split_inner(bptree, parent);
            }
            _save_node(bptree, parent);
            _free_node(bptree, parent);
        }
        else
        {
            parent = _new_inner_node(bptree, ROOT_ADDR, 0);
            _insert_into_inner(parent, node->keys[spliter_idx], node->next_node);
            node->addr = _bptree_next_addr(bptree);
            parent->children[0] = node->addr;
            node->parent = parent->addr;
            bptree->root = parent;
            _save_node(bptree, parent);
        }
    }
    
    _copy_node(node, new_node, spliter_idx);
    node->key_num = spliter_idx;
    _save_node(bptree, new_node);
    _free_node(bptree, new_node);
}

void bptree_init(bptree_t *bptree, Buffer *buffer)
{
    bptree->buffer = buffer;
    bptree->root = _read_node(bptree, ROOT_ADDR);
    if (bptree->root == NULL)
    {
        bptree->root = _new_leaf_node(bptree, ROOT_ADDR, 0);
        _save_node(bptree, bptree->root);
    }
    bptree->curr_addr = ROOT_ADDR;
}

void bptree_insert(bptree_t *bptree, int key, addr_t addr)
{
    node_t *node = _bptree_query(bptree, key);
    _insert_into_leaf(node, key, addr);

    if (node->key_num >= MAX_KEY_NUM)
    {
        _bptree_split_leaf(bptree, node);
    }
    _save_node(bptree, node);
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

void bptree_free(bptree_t *bptree)
{

}