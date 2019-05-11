#include <time.h>
#include <stdio.h>

#include "common.h"
#include "bptree.h"
#include "../lib/libextmem.h"

#define R_BLK_NUM 16
#define S_BLK_NUM 32
#define R_ADDR_PREFIX 'R' * 100
#define S_ADDR_PREFIX 'S' * 100

int gen_blocks(Buffer *buffer, bptree_t *bptree);

int main(int argc, char *argv[])
{
    Buffer _buffer;
    Buffer *buffer = &_buffer;
    initBuffer(520, 64, buffer);

    bptree_t bptree;
    bptree_init(&bptree, buffer);

    srand((unsigned int)time(NULL));
    gen_blocks(buffer, &bptree);

    return 0;
}

int gen_blocks(Buffer *buffer, bptree_t *bptree)
{
    addr_t blk_addr;
    Block *block = (Block *)getNewBlockInBuffer(buffer);

    for (int blk_num = 0; blk_num < R_BLK_NUM; blk_num++)
    {
        blk_addr = R_ADDR_PREFIX + blk_num;
        for (int i = 0; i < TUPLES_PER_BLK; i++)
        {
            gen_R(&(block->R_data[i]));
            bptree_insert(bptree, block->R_data[i].A, blk_addr);
        }

        if (blk_num < R_BLK_NUM - 1)
        {
            block->next_blk = blk_num + 1;
        }
        else
        {
            block->next_blk = 0;
        }

        writeBlockToDisk((unsigned char *)block, blk_addr, buffer);
        freeBlockInBuffer((unsigned char *)block, buffer);
        block = (Block *)getNewBlockInBuffer(buffer);
    }

    bptree_print(bptree);

    for (int blk_num = 0; blk_num < S_BLK_NUM; blk_num++)
    {
        for (int i = 0; i < TUPLES_PER_BLK; i++)
        {
            gen_S(&(block->S_data[i]));
        }

        if (blk_num < S_BLK_NUM - 1)
        {
            block->next_blk = blk_num + 1;
        }
        else
        {
            block->next_blk = 0;
        }

        writeBlockToDisk((unsigned char *)block, S_ADDR_PREFIX + blk_num, buffer);
        freeBlockInBuffer((unsigned char *)block, buffer);
        block = (Block *)getNewBlockInBuffer(buffer);
    }
    freeBlockInBuffer((unsigned char *)block, buffer);
    return 0;
}