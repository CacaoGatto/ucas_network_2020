//
// Created by cacao on 2020/6/4.
//

#ifndef PRJ09_BASE_H
#define PRJ09_BASE_H

#include "types.h"

typedef struct node {
    struct node *son[2];
    u32 iface;
    u32 dest;
    int match;
} node_t;

node_t *init_root_base();

void put_base(node_t *root, u32 iface, u32 mask, u32 dest);

node_t *get_base(node_t *root, u32 dest);

unsigned calc_space_base();

#endif //PRJ09_BASE_H
