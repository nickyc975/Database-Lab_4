#include <stdio.h>

#include "database.h"

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