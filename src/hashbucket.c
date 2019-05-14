#include <stdio.h>
#include <stdlib.h>

#include "hashbucket.h"

typedef struct bukt_struct
{
    int key;
    addr_t value;
    struct bukt_struct *next_bukt;
} bukt_t;

struct hashbukt_struct
{
    int bukt_num;
    int (* hash)(int key);
    bukt_t **buckets;
};

struct bukt_iter_struct
{
    int curr_key;
    addr_t curr_value;
    bukt_t *next_bukt;
};

hashbukt_t *new_hashbukt(int bukt_num, int (* hash)(int key))
{
    hashbukt_t *hashbukt = (hashbukt_t *)malloc(sizeof(hashbukt_t));
    hashbukt->bukt_num = bukt_num;
    hashbukt->hash = hash;
    hashbukt->buckets = (bukt_t **)malloc(sizeof(bukt_t *) * bukt_num);
    for (int i = 0; i < bukt_num; i++)
    {
        hashbukt->buckets[i] = NULL;
    }
    return hashbukt;
}

void hashbukt_put(hashbukt_t *hashbukt, int key, addr_t value)
{
    int bukt_no = hashbukt->hash(key);
    if (bukt_no >= 0 && bukt_no < hashbukt->bukt_num)
    {
        bukt_t *bukt = (bukt_t *)malloc(sizeof(bukt_t));
        bukt->key = key;
        bukt->value = value;
        bukt->next_bukt = hashbukt->buckets[bukt_no];
        hashbukt->buckets[bukt_no] = bukt;
        return;
    }
    printf("hash value %d out of interval [0, %d)!", bukt_no, hashbukt->bukt_num);
    exit(1);
}

bukt_iter_t *hashbukt_get(hashbukt_t *hashbukt, int key)
{
    int bukt_no = hashbukt->hash(key);
    if (bukt_no >= 0 && bukt_no < hashbukt->bukt_num)
    {
        bukt_iter_t *iter = (bukt_iter_t *)malloc(sizeof(bukt_iter_t));
        iter->next_bukt = hashbukt->buckets[bukt_no];
        return iter;
    }
    printf("hash value %d out of interval [0, %d)!", bukt_no, hashbukt->bukt_num);
    exit(1);
}

bukt_iter_t *hashbukt_bukt_at(hashbukt_t *hashbukt, int index)
{
    if (index >= 0 && index < hashbukt->bukt_num)
    {
        bukt_iter_t *iter = (bukt_iter_t *)malloc(sizeof(bukt_iter_t));
        iter->next_bukt = hashbukt->buckets[index];
        return iter;
    }
    printf("index %d out of interval [0, %d)!", index, hashbukt->bukt_num);
    exit(1);
}

int bukt_has_next(bukt_iter_t *iter)
{
    return iter->next_bukt != NULL;
}

void bukt_next(bukt_iter_t *iter)
{
    if (iter->next_bukt)
    {
        iter->curr_key = iter->next_bukt->key;
        iter->curr_value = iter->next_bukt->value;
        iter->next_bukt = iter->next_bukt->next_bukt;
    }
    printf("no more buckets!\n");
    exit(1);
}

int bukt_curr_key(bukt_iter_t *iter)
{
    return iter->curr_key;
}

addr_t bukt_curr_value(bukt_iter_t *iter)
{
    return iter->curr_value;
}

void free_hashbukt(hashbukt_t *hashbukt)
{
    if (hashbukt)
    {
        if (hashbukt->buckets)
        {
            bukt_t *bukt, *next_bukt;
            for (int i = 0; i < hashbukt->bukt_num; i++)
            {
                bukt = hashbukt->buckets[i];
                while (bukt)
                {
                    next_bukt = bukt->next_bukt;
                    free(bukt);
                    bukt = next_bukt;
                }
            }
            free(hashbukt->buckets);
        }
        free(hashbukt);
    }
}

void free_bukt_iter(bukt_iter_t *iter)
{
    if (iter)
    {
        free(iter);
    }
}