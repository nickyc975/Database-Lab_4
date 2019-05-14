#ifndef HASH_BUCKET_H
#define HASH_BUCKET_H

#include "../lib/libextmem.h"

typedef struct hashbukt_struct hashbukt_t;

typedef struct bukt_iter_struct bukt_iter_t;

hashbukt_t *new_hashbukt(int bukt_num, int (* hash)(int key));

void hashbukt_put(hashbukt_t *hashbukt, int key, addr_t value);

bukt_iter_t *hashbukt_get(hashbukt_t *hashbukt, int key);

int bukt_has_next(bukt_iter_t *iter);

void bukt_next(bukt_iter_t *iter);

int bukt_curr_key(bukt_iter_t *iter);

addr_t bukt_curr_value(bukt_iter_t *iter);

void free_hashbukt(hashbukt_t *hashbukt);

void free_bukt_iter(bukt_iter_t *iter);

#endif