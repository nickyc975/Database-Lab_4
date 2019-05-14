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

int project(database_t *database, addr_t base_addr)
{
    int count = 0;
    block_t *block = new_blk(database->buffer);
    key_iter_t *iter = new_key_iter(&(database->bptree_meta), database->buffer);
    while(has_next_key(iter))
    {
        block->tuples[block->tuple_num] = next_key(iter);
        block->tuple_num++;
        count++;
        if (block->tuple_num >= TUPLES_PER_BLK * sizeof(R) / sizeof(int))
        {
            base_addr++;
            block->next_blk = base_addr;
            save_blk(database->buffer, block, base_addr - 1, 0);
            block->tuple_num = 0;
        }
    }

    block->next_blk = 0;
    save_blk(database->buffer, block, base_addr, 1);
    free_key_iter(iter);
    return count;
}

int nest_loop_join(database_t *R_db, database_t *S_db, addr_t base_addr)
{
    int count = 0;
    int S_blks_len = 0;
    block_t *R_blk, *S_blks[6];
    block_t *blk_buf = new_blk(R_db->buffer);
    addr_t next_R_blk_addr = R_db->head_blk_addr, next_S_blk_addr = S_db->head_blk_addr;

    while(next_R_blk_addr)
    {
        int S_blk_num = 0;
        R_blk = read_blk(R_db->buffer, next_R_blk_addr);
        while(S_blk_num < S_db->blk_num)
        {
            while(S_blks_len < 6)
            {
                if (next_S_blk_addr)
                {
                    S_blks[S_blks_len] = read_blk(S_db->buffer, next_S_blk_addr);
                }
                else
                {
                    S_blks[S_blks_len] = read_blk(S_db->buffer, S_db->head_blk_addr);
                }
                next_S_blk_addr = S_blks[S_blks_len]->next_blk;
                S_blks_len++;
            }

            for (int i = 0; i < S_blks_len && S_blk_num < S_db->blk_num; i++, S_blk_num++)
            {
                for (int j = 0; j < R_blk->tuple_num; j++)
                {
                    for (int k = 0; k < S_blks[i]->tuple_num; k++)
                    {
                        if (R_blk->R_tuples[j].key == S_blks[i]->S_tuples[k].key)
                        {
                            blk_buf->joined_tuples[blk_buf->tuple_num].R_tuple = R_blk->R_tuples[j];
                            blk_buf->joined_tuples[blk_buf->tuple_num].S_tuple = S_blks[i]->S_tuples[k];
                            blk_buf->tuple_num++;
                            count++;
                            if (blk_buf->tuple_num >= TUPLES_PER_BLK / 2)
                            {
                                base_addr++;
                                blk_buf->next_blk = base_addr;
                                save_blk(R_db->buffer, blk_buf, base_addr - 1, 0);
                                blk_buf->tuple_num = 0;
                            }
                        }
                    }
                }
            }

            if (S_blk_num < S_db->blk_num)
            {
                while(S_blks_len > 0)
                {
                    S_blks_len--;
                    free_blk(S_db->buffer, S_blks[S_blks_len]);
                }
            }
        }
        next_R_blk_addr = R_blk->next_blk;
        free_blk(R_db->buffer, R_blk);
    }

    while(S_blks_len > 0)
    {
        S_blks_len--;
        free_blk(S_db->buffer, S_blks[S_blks_len]);
    }
    
    blk_buf->next_blk = 0;
    save_blk(R_db->buffer, blk_buf, base_addr, 1);
    return count;
}

int sort_merge_join(database_t *R_db, database_t *S_db, addr_t base_addr)
{
    int count = 0, R_key = 0, S_key = 0;
    block_t *R_curr_blk = NULL, *S_curr_blk = NULL;
    addr_t R_curr_blk_addr = 0, S_curr_blk_addr = 0;
    addr_t R_value_blk_addr, S_value_blk_addr;
    block_t *blk_buf = new_blk(R_db->buffer);
    key_iter_t *R_iter = new_key_iter(&(R_db->bptree_meta), R_db->buffer);
    key_iter_t *S_iter = new_key_iter(&(S_db->bptree_meta), S_db->buffer);

    while(has_next_key(R_iter) || has_next_key(S_iter))
    {
        if (R_key == S_key && has_next_key(R_iter) && has_next_key(S_iter))
        {
            R_key = next_key(R_iter);
            S_key = next_key(S_iter);
        }
        else if (R_key < S_key && has_next_key(R_iter))
        {
            R_key = next_key(R_iter);
        }
        else if (R_key > S_key && has_next_key(S_iter))
        {
            S_key = next_key(S_iter);
        }
        else
        {
            break;
        }

        if (R_key == S_key)
        {
            R_value_blk_addr = curr_blk_addr(R_iter);
            S_value_blk_addr = curr_blk_addr(S_iter);
            value_blk_iter_t *R_value_iter = new_value_blk_iter(R_value_blk_addr, R_db->buffer);

            while (has_next_value(R_value_iter))
            {
                block_t *R_blk;
                addr_t R_value = next_value(R_value_iter);
                addr_t R_blk_addr = get_blk_addr(R_value);
                int R_blk_offset = get_blk_offset(R_value);

                if (R_blk_addr == R_curr_blk_addr)
                {
                    R_blk = R_curr_blk;
                }
                else
                {
                    R_blk = read_blk(R_db->buffer, R_blk_addr);
                    if (R_curr_blk)
                    {
                        free_blk(R_db->buffer, R_curr_blk);
                    }
                    R_curr_blk_addr = R_blk_addr;
                    R_curr_blk = R_blk;
                }
                
                 value_blk_iter_t *S_value_iter = new_value_blk_iter(S_value_blk_addr, S_db->buffer);
                 while (has_next_value(S_value_iter))
                 {
                    block_t *S_blk;
                    addr_t S_value = next_value(S_value_iter);
                    addr_t S_blk_addr = get_blk_addr(S_value);
                    int S_blk_offset = get_blk_offset(S_value);

                    if (S_blk_addr == S_curr_blk_addr)
                    {
                        S_blk = S_curr_blk;
                    }
                    else
                    {
                        S_blk = read_blk(S_db->buffer, S_blk_addr);
                        if (S_curr_blk)
                    {
                        free_blk(S_db->buffer, S_curr_blk);
                    }
                        S_curr_blk_addr = S_blk_addr;
                        S_curr_blk = S_blk;
                    }

                    blk_buf->joined_tuples[blk_buf->tuple_num].R_tuple = R_blk->R_tuples[R_blk_offset];
                    blk_buf->joined_tuples[blk_buf->tuple_num].S_tuple = S_blk->S_tuples[S_blk_offset];
                    blk_buf->tuple_num++;
                    count++;
                    if (blk_buf->tuple_num >= TUPLES_PER_BLK / 2)
                    {
                        base_addr++;
                        blk_buf->next_blk = base_addr;
                        save_blk(R_db->buffer, blk_buf, base_addr - 1, 0);
                        blk_buf->tuple_num = 0;
                    }
                 }
                 free_value_blk_iter(S_value_iter);
            }
            free_value_blk_iter(R_value_iter);
        }
    }

    if (R_curr_blk)
    {
        free_blk(R_db->buffer, R_curr_blk);
    }

    if (S_curr_blk)
    {
        free_blk(S_db->buffer, S_curr_blk);
    }

    free_key_iter(R_iter);
    free_key_iter(S_iter);

    blk_buf->next_blk = 0;
    save_blk(R_db->buffer, blk_buf, base_addr, 1);
    return count;
}

int hash_join(database_t *R_db, database_t *S_db, addr_t base_addr)
{
    int count = 0;
    return count;
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