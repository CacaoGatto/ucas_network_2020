//
// Created by cacao on 2020/6/6.
//

#ifndef PRJ09_POPTRIE_H
#define PRJ09_POPTRIE_H

#include "types.h"

#define K 6
#define SEC_MASK 0xfc000000

typedef struct leaf_node {
    u32 iface;
    u32 mask;
} leaf_t;

typedef struct internal_node {
    u64 leafvec;
    u64 vector;
    struct internal_node *child;
    struct leaf_node *leaf;
} internal_t;


internal_t *init_root_poptrie();

leaf_t *get_poptrie(internal_t *root, u32 dest);

void put_poptrie(internal_t *root, u32 dest, u32 mask_copy, u32 iface);

unsigned calc_space_poptrie();

#endif //PRJ09_POPTRIE_H
