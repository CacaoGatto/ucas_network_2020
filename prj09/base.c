//
// Created by cacao on 2020/6/4.
//

#include "base.h"

#include <stdlib.h>
#include <stdio.h>

unsigned counter_base = 0;

static node_t *malloc_base() {
    counter_base++;
    node_t* tmp = (node_t*)malloc(sizeof(node_t));
    return tmp;
}

node_t *init_root_base() {
    node_t *temp = malloc_base();
    temp->son[0] = temp->son[1] = NULL;
    return temp;
}

void put_base(node_t* root, u32 iface, u32 mask, u32 dest) {
    int got = 1, depth = 0;
    u32 bit_idx = 0, bit_chk = 0x80000000;
    node_t *current = root;
    while (depth++ < mask) {
        bit_idx = (bit_chk & dest) >> (32 - depth);
        if (current->son[bit_idx] == NULL) {
            got = 0;
            break;
        }
        current = current->son[bit_idx];
        bit_chk = bit_chk >> 1;
    }
    if (got) {
        current->match = 1;
        current->iface = iface;
        current->mask = mask;
        current->dest = dest;
        return;
    }
    while (depth < mask) {
        current->son[bit_idx] = malloc_base();
        current = current->son[bit_idx];
        current->son[0] = current->son[1] = NULL;
        current->match = 0;
        bit_chk = bit_chk >> 1;
        depth++;
        bit_idx = (bit_chk & dest) >> (32 - depth);
    }
    current->son[bit_idx] = malloc_base();
    current = current->son[bit_idx];
    current->son[0] = current->son[1] = NULL;
    current->match = 1;
    current->iface = iface;
    current->mask = mask;
    current->dest = dest;
}

node_t *get_base(node_t* root, u32 mask, u32 dest) {
    int depth = 0;
    u32 bit_idx = 0, bit_chk = 0x80000000;
    node_t *current = root, *result = NULL;
    while (depth++ < mask) {
        bit_idx = (bit_chk & dest) >> (32 - depth);
        if (current->son[bit_idx] == NULL) break;
        current = current->son[bit_idx];
        if (current->match) result = current;
        bit_chk = bit_chk >> 1;
    }
    return result;
}

unsigned calc_space_base() {
    return (counter_base * sizeof(node_t));
}
