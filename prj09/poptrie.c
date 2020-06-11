//
// Created by cacao on 2020/6/6.
//

#include "poptrie.h"

#include <stdlib.h>
#include <string.h>

unsigned counter_leaf = 0;
unsigned counter_internal = 0;
leaf_t cache[64];

static int exp2(int level) {
    int result = 1;
    for (int i = 0; i < level; ++i) {
        result *= 2;
    }
    return result;
}

static int count_one_bit(u64 num) {
    int counter = 0;
    for (int i = 0; i < 64; ++i) {
        counter += ((num & 0x1) == 0x1);
        num = num >> 1;
    }
    return counter;
}

static leaf_t *malloc_leaf(int size) {
    counter_leaf += size;
    leaf_t *tmp = (leaf_t *) malloc(sizeof(leaf_t) * size);
    memset(tmp, 0, sizeof(leaf_t));
    return tmp;
}

static internal_t *malloc_internal(int size) {
    counter_internal += size;
    internal_t *tmp = (internal_t *) malloc(sizeof(internal_t) * size);
    memset(tmp, 0, sizeof(internal_t) * size);
    return tmp;
}

static void refill_leaf(internal_t *node, u32 iface, u32 mask) {
    int leaf_num = count_one_bit(node->leafvec);
    leaf_t *leaf_tmp = node->leaf;
    for (int i = 0; i < leaf_num; ++i) {
        if (leaf_tmp->mask < mask) {
            leaf_tmp->mask = mask;
            leaf_tmp->iface = iface;
        }
        leaf_tmp++;
    }
    int internal_num = count_one_bit(node->vector);
    internal_t *internal_tmp = node->child;
    for (int j = 0; j < internal_num; ++j) {
        refill_leaf(internal_tmp, iface, mask);
        internal_tmp++;
    }
}

internal_t *init_root_poptrie() {
    internal_t *tmp = malloc_internal(1);
    tmp->leafvec = 0x1;
    tmp->leaf = malloc_leaf(1);
    return tmp;
}

leaf_t *get_poptrie(internal_t *root, u32 dest) {
    u32 key, mask = 32;
    int idx[2];
    internal_t *current = root;
    leaf_t *result = NULL;
    while (mask > 0) {
        key = (dest & SEC_MASK) >> (32 - K);
        mask -= K;
        dest = dest << K;
        idx[0] = idx[1] = 0;
        u64 vector = current->vector;
        u64 leafvec = current->leafvec;
        u16 bit, lbit;
        for (int i = 0; i < key; ++i) {
            bit = vector & 0x1;
            lbit = leafvec & 0x1;
            idx[bit] += (bit | lbit);
            vector = vector >> 1;
            leafvec = leafvec >> 1;
        }
        bit = vector & 0x1;
        lbit = leafvec & 0x1;
        idx[bit] += (bit | lbit);
        if (!bit) {
            result = current->leaf;
            result += idx[0] - 1;
            return result;
        } else {
            current = current->child;
            //if (!current) return NULL;
            current += idx[1] - 1;
        }
    }
    return result;
}

void put_poptrie(internal_t *root, u32 dest, u32 mask, u32 iface) {
    dest = dest >> (32 - mask);
    dest = dest << (32 - mask);
    u32 key, mask_copy = mask;
    int idx[2];
    internal_t *current = root;
    for (; mask_copy > K; mask_copy -= K) {
        key = (dest & SEC_MASK) >> (32 - K);
        dest = dest << K;
        idx[0] = idx[1] = 0;
        u64 vector = current->vector;
        u64 leafvec = current->leafvec;
        u16 bit, lbit;
        for (int i = 0; i < key; ++i) {
            bit = vector & 0x1;
            lbit = leafvec & 0x1;
            idx[bit] += (bit | lbit);
            vector = vector >> 1;
            leafvec = leafvec >> 1;
        }
        bit = vector & 0x1;
        lbit = leafvec & 0x1;
        idx[bit] += (bit | lbit);
        if (!bit) {
            int one_bit = count_one_bit(current->vector);
            u64 set_bit = 0x1L;
            set_bit = set_bit << key;
            current->vector |= set_bit;
            current->leafvec &= (~set_bit);
            u32 iface_tmp = (current->leaf + idx[0])->iface;
            u32 mask_tmp = (current->leaf + idx[0])->mask;
            if (lbit) {
                u16 tail_v = 1, tail_l = 0;
                u32 i;
                for (i = key + 1; i < 64; ++i) {
                    vector = vector >> 1;
                    leafvec = leafvec >> 1;
                    tail_l = leafvec & 0x1;
                    tail_v = vector & 0x1;
                    if (!tail_v) break;
                }
                if (!tail_v && !tail_l) {
                    set_bit = 0x1;
                    set_bit = set_bit << i;
                    current->leafvec |= set_bit;
                } else {
                    int l_one_bit = count_one_bit(current->leafvec);
                    leaf_t *new_leaf = malloc_leaf(l_one_bit);
                    memcpy(new_leaf, current->leaf, sizeof(leaf_t) * (idx[0] - 1));
                    memcpy(new_leaf + idx[0] - 1, current->leaf + idx[0],
                           sizeof(leaf_t) * (l_one_bit - idx[0] + 1));
                    unsigned size = sizeof(*(current->leaf));
                    counter_leaf -= size / sizeof(leaf_t);
                    free(current->leaf);
                    current->leaf = new_leaf;
                }
            }
            internal_t *new_child = malloc_internal(one_bit + 1);
            memcpy(new_child, current->child, sizeof(internal_t) * idx[1]);
            memcpy(new_child + idx[1], current->child + idx[1] - 1,
                   sizeof(internal_t) * (one_bit - idx[1]));
            internal_t *new_internal = new_child + idx[1];
            new_internal->leafvec = 0x1;
            new_internal->leaf = malloc_leaf(1);
            new_internal->leaf->iface = iface_tmp;
            new_internal->leaf->mask = mask_tmp;
            unsigned size = sizeof(*(current->child));
            counter_internal -= size / sizeof(internal_t);
            free(current->child);
            current->child = new_child;
            counter_internal++;
        }
        current = current->child;
        current += idx[1] - bit;
    }

    int cache_idx = 0;
    key = (dest & SEC_MASK) >> (32 - K);
    idx[0] = idx[1] = 0;
    int range = exp2(K - mask_copy);
    u64 vector = current->vector;
    u64 leafvec = current->leafvec;
    u16 bit, lbit;
    for (int i = 0; i < key; ++i) {
        bit = vector & 0x1;
        lbit = leafvec & 0x1;
        if (!bit && lbit) {
            memcpy(cache + cache_idx, current->leaf + idx[0], sizeof(leaf_t));
            cache_idx++;
        }
        idx[bit] += (bit | lbit);
        vector = vector >> 1;
        leafvec = leafvec >> 1;
    }
    int first = 1;
    u64 bit_edit = 0x1L << key;
    for (int i = 0; i < range; ++i) {
        bit = vector & 0x1;
        lbit = leafvec & 0x1;
        idx[bit] += (bit | lbit);
        vector = vector >> 1;
        leafvec = leafvec >> 1;
        if (bit) refill_leaf(current->child + idx[1] - 1, iface, mask);
        else if (mask < (current->leaf + idx[0] - 1)->mask) {
            first = 1;
            if (lbit) {
                memcpy(cache + cache_idx, current->leaf + idx[0] - 1, sizeof(leaf_t));
                cache_idx++;
            }
        }
        else {
            if (first) {
                current->leafvec |= bit_edit;
                first = 0;
                cache[cache_idx].mask = mask;
                cache[cache_idx].iface = iface;
                cache_idx++;
            } else current->leafvec &= (~bit_edit);
        }
        bit_edit = bit_edit << 1;
    }
    u16 tail_v = 1, tail_l = 0;
    u32 i;
    first = 1;
    for (i = key + range; i < 64; ++i) {
        tail_l = leafvec & 0x1;
        tail_v = vector & 0x1;
        if (first) {
            if (!tail_v) {
                if (!tail_l) {
                    current->leafvec |= bit_edit;
                    memcpy(cache + cache_idx, current->leaf + idx[0] - 1, sizeof(leaf_t));
                    cache_idx++;
                } else {
                    memcpy(cache + cache_idx, current->leaf + idx[0], sizeof(leaf_t));
                    cache_idx++;
                    idx[0]++;
                }
                first = 0;
            }
        } else if (!tail_v && tail_l) {
            memcpy(cache + cache_idx, current->leaf + idx[0], sizeof(leaf_t));
            cache_idx++;
            idx[0]++;
        }
        vector = vector >> 1;
        leafvec = leafvec >> 1;
        bit_edit = bit_edit << 1;
    }

    leaf_t *new_leaf = malloc_leaf(cache_idx);
    memcpy(new_leaf, cache, sizeof(leaf_t) * cache_idx);
    unsigned size = sizeof(*(current->leaf));
    counter_leaf -= size / sizeof(leaf_t);
    free(current->leaf);
    current->leaf = new_leaf;
}

unsigned calc_space_poptrie() {
    return (counter_leaf * sizeof(leaf_t) + counter_internal * sizeof(internal_t));
}
