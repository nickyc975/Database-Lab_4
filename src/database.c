#include <math.h>
#include <stdio.h>

#include "database.h"

int _read_value_blks(database_t *database, addr_t value_blk_addr, addr_t base_addr);

addr_t next_blk_addr(addr_t addr)
{
    return (addr & (~0x7)) + 0x8;
}

addr_t get_blk_addr(addr_t addr)
{
    return addr & (~0x7);
}

addr_t get_blk_offset(addr_t addr)
{
    return addr & 0x7;
}

block_t *new_blk(Buffer *buffer)
{
    block_t *block = (block_t *)getNewBlockInBuffer(buffer);
    block->tuple_num = 0;
    block->next_blk = 0;
    return block;
}

block_t *read_blk(Buffer *buffer, addr_t blk_addr)
{
    return (block_t *)readBlockFromDisk(blk_addr, buffer);
}

int save_blk(Buffer *buffer, block_t *block, addr_t blk_addr, int free_after_save)
{
    int result = writeBlockToDisk((unsigned char *)block, blk_addr, buffer);
    if (free_after_save)
    {
        free_blk(buffer, block);
    }
    return result;
}

void free_blk(Buffer *buffer, block_t *block)
{
    freeBlockInBuffer((unsigned char *)block, buffer);
}

int linear_search(database_t *database, int key, addr_t base_addr)
{
    int count = 0;
    block_t *block, *blk_buf;
    addr_t addr = database->head_blk_addr;

    blk_buf = new_blk(database->buffer);
    while(addr)
    {
        block = read_blk(database->buffer, addr);
        for (int i = 0; i < block->tuple_num; i++)
        {
            switch (database->type)
            {
                case R_T:
                    if (block->R_tuples[i].key == key)
                    {
                        blk_buf->R_tuples[blk_buf->tuple_num] = block->R_tuples[i];
                        break;
                    }
                    continue;
                case S_T:
                    if (block->S_tuples[i].key == key)
                    {
                        blk_buf->S_tuples[blk_buf->tuple_num] = block->S_tuples[i];
                        break;
                    }
                    continue;
                default:
                    printf("Unknown database type: %d\n", database->type);
                    exit(1);
            }

            count++;
            blk_buf->tuple_num++;
            if (blk_buf->tuple_num >= TUPLES_PER_BLK)
            {
                base_addr++;
                blk_buf->next_blk = base_addr;
                save_blk(database->buffer, blk_buf, base_addr - 1, 0);
                blk_buf->tuple_num = 0;
            }
        }

        addr = block->next_blk;
        free_blk(database->buffer, block);
    }

    blk_buf->next_blk = 0;
    save_blk(database->buffer, blk_buf, base_addr, 1);
    return count;
}

int binary_search(database_t *database, int key, addr_t base_addr)
{
    node_t *node;
    addr_t node_addr, value_blk_addr = 0;
    bptree_t *bptree = &(bptree_t){.buffer = database->buffer};
    int last_pos = database->bptree_meta.leaf_num - 1, pos = (last_pos + 1) / 2, temp;

    while (pos >= 0 && pos < database->bptree_meta.leaf_num)
    {
        node_addr = database->bptree_meta.leaf_addrs[pos];
        node = read_node(bptree, node_addr);
        if (key < node_minkey(node))
        {
            temp = pos;
            pos -= (abs(last_pos - temp) + 1) / 2;
            last_pos = temp;
        }
        else if (key <= node_maxkey(node))
        {
            value_blk_addr = node_get(node, key);
            pos = -1;
        }
        else
        {
            temp = pos;
            pos += (abs(last_pos - temp) + 1) / 2;
            last_pos = temp;
        }
        free_node(bptree, node);
    }

    return _read_value_blks(database, value_blk_addr, base_addr);
}

int index_search(database_t *database, int key, addr_t base_addr)
{
    bptree_t bptree;
    bptree_init(&bptree, &(database->bptree_meta), database->buffer);
    addr_t value_blk_addr = bptree_query(&bptree, key);
    bptree_free(&bptree);
    return _read_value_blks(database, value_blk_addr, base_addr);
}

int _read_value_blks(database_t *database, addr_t value_blk_addr, addr_t base_addr)
{
    int count = 0;
    value_blk_t *value_blk;
    addr_t curr_blk_addr = 0, blk_addr, blk_offset;
    bptree_t *bptree = &(bptree_t){.buffer = database->buffer};
    block_t *block = NULL, *blk_buf = new_blk(database->buffer);

    while(value_blk_addr)
    {
        value_blk = read_value_blk(bptree, value_blk_addr);
        for (int i = 0; i < value_blk->value_num; i++)
        {
            blk_addr = get_blk_addr(value_blk->values[i]);
            blk_offset = get_blk_offset(value_blk->values[i]);
            if (!block || blk_addr != curr_blk_addr)
            {
                curr_blk_addr = blk_addr;
                if (block)
                {
                    free_blk(database->buffer, block);
                }
                block = read_blk(database->buffer, curr_blk_addr);
            }

            if (blk_offset >= block->tuple_num)
            {
                printf("Invalid block offset %d, beyond tuple num %d\n", blk_offset, block->tuple_num);
                exit(1);
            }
            
            switch (database->type)
            {
                case R_T:
                    blk_buf->R_tuples[blk_buf->tuple_num] = block->R_tuples[blk_offset];
                    break;
                case S_T:
                    blk_buf->S_tuples[blk_buf->tuple_num] = block->S_tuples[blk_offset];
                    break;
                default:
                    printf("Unknown database type: %d\n", database->type);
                    exit(1);
            }

            count++;
            blk_buf->tuple_num++;
            if (blk_buf->tuple_num >= TUPLES_PER_BLK)
            {
                base_addr++;
                blk_buf->next_blk = base_addr;
                save_blk(database->buffer, blk_buf, base_addr - 1, 0);
                blk_buf->tuple_num = 0;
            }
        }
        value_blk_addr = value_blk->next_blk_addr;
        free_value_blk(bptree, value_blk);
    }

    if (block)
    {
        free_blk(database->buffer, block);
    }

    blk_buf->next_blk = 0;
    save_blk(database->buffer, blk_buf, base_addr, 1);
    return count;
}