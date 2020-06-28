//
// Created by cacao on 2020/6/6.
//

#include "poptrie.h"

#include <stdlib.h>
#include <string.h>

unsigned counter_leaf = 0;
unsigned counter_internal = 0;
leaf_t cache[64];
internal_t *direct_pointing[0x1000];

static int count_one_bit(u64 num) {
    int counter = 0;
    for (int i = 0; i < 64; ++i) {
        counter += ((num & 0x1) == 0x1);
        num = num >> 1;
    }
    return counter;
}

static void renew_pointing(u32 head, u64 vec, internal_t *new_child) {
    u32 idx = 0;
    for (int i = 0; i < 64; ++i) {
        u32 bit = vec & 0x1;
        if (bit) {
            direct_pointing[head + i] = new_child + idx;
            idx++;
        }
        vec = vec >> 1;
    }
}

static leaf_t *malloc_leaf(int size) {
    //counter_leaf += size;
    leaf_t *tmp = (leaf_t *) malloc(sizeof(leaf_t) * size);
    memset(tmp, 0, sizeof(leaf_t));
    return tmp;
}

static internal_t *malloc_internal(int size) {
    counter_internal += 1;
    internal_t *tmp = (internal_t *) malloc(sizeof(internal_t) * size);
    memset(tmp, 0, sizeof(internal_t) * size);
    return tmp;
}

internal_t *init_root_poptrie() {
    internal_t *tmp = malloc_internal(1);
    tmp->leafvec = 0x1;
    tmp->leaf = malloc_leaf(1);
    counter_leaf++;
    memset(direct_pointing, 0, sizeof(direct_pointing));
    return tmp;
}

leaf_t *get_poptrie(internal_t *root, u32 dest) {
    u32 key, mask = 32;
    int idx[2];
    leaf_t *result = NULL;
    internal_t *current = direct_pointing[dest >> 20];
    if (!current || mask <= 12) current = root;
    else {
        mask -= 12;
        dest = dest << 12;
    }
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
    u32 head = (dest & SEC_MASK) >> (32 - K);
    head = head << K;
    int idx[2];
    int level = 2;
    internal_t *current = direct_pointing[dest >> 20];
    if (!current || mask <= 12) {
        current = root;
        level = 0;
    } else {
        mask_copy -= 12;
        dest = dest << 12;
    }
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
            u32 iface_tmp = (current->leaf + idx[0] - 1)->iface;
            u32 mask_tmp = (current->leaf + idx[0] - 1)->mask;
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
                    set_bit = 0x1L;
                    set_bit = set_bit << i;
                    current->leafvec |= set_bit;
                } else {
                    int l_one_bit = count_one_bit(current->leafvec);
                    leaf_t *new_leaf = malloc_leaf(l_one_bit);
                    memcpy(new_leaf, current->leaf, sizeof(leaf_t) * (idx[0] - 1));
                    memcpy(new_leaf + idx[0] - 1, current->leaf + idx[0],
                           sizeof(leaf_t) * (l_one_bit - idx[0] + 1));
                    free(current->leaf);
                    current->leaf = new_leaf;
                    counter_leaf--;
                }
            }
            internal_t *new_child = malloc_internal(one_bit + 1);
            memcpy(new_child, current->child, sizeof(internal_t) * idx[1]);
            memcpy(new_child + idx[1] + 1, current->child + idx[1],
                   sizeof(internal_t) * (one_bit - idx[1]));
            internal_t *new_internal = new_child + idx[1];
            new_internal->leafvec = 0x1;
            new_internal->leaf = malloc_leaf(1);
            new_internal->leaf->iface = iface_tmp;
            new_internal->leaf->mask = mask_tmp;
            free(current->child);
            if (level == 1) renew_pointing(head, current->vector, new_child);
            current->child = new_child;
            counter_leaf++;
        }
        current = current->child;
        current += idx[1] - bit;
        level++;
    }

    key = (dest & SEC_MASK) >> (32 - K);
    idx[0] = idx[1] = 0;
    int range = 0x1 << (K - mask_copy);
    u64 vector = current->vector;
    u64 leafvec = current->leafvec;
    u16 bit, lbit;
    u32 last_iface = 0, last_mask = 0;
    int cache_idx = 0;
    for (int i = 0; i < key; ++i) {
        bit = vector & 0x1;
        lbit = leafvec & 0x1;
        if (!bit && lbit) {
            last_iface = (current->leaf + idx[0])->iface;
            last_mask = (current->leaf + idx[0])->mask;
            memcpy(cache + cache_idx, current->leaf + idx[0], sizeof(leaf_t));
            cache_idx++;
            idx[0]++;
        }
        vector = vector >> 1;
        leafvec = leafvec >> 1;
    }
    int first = 1;
    u64 bit_edit = 0x1L << key;
    for (int i = 0; i < range; ++i) {
        bit = vector & 0x1;
        lbit = leafvec & 0x1;
        if (!bit && (first || lbit)) {
            if (lbit) {
                last_iface = (current->leaf + idx[0])->iface;
                last_mask = (current->leaf + idx[0])->mask;
                idx[0]++;
            }
            first = 0;
            if (mask >= last_mask) {
                current->leafvec |= bit_edit;
                cache[cache_idx].mask = mask;
                cache[cache_idx].iface = iface;
                cache_idx++;
            }
        }
        vector = vector >> 1;
        leafvec = leafvec >> 1;
        bit_edit = bit_edit << 1;
    }
    first = !first;
    u32 i;
    for (i = key + range; i < 64; ++i) {
        lbit = leafvec & 0x1;
        bit = vector & 0x1;
        if (!bit) {
            if (first && !lbit) {
                current->leafvec |= bit_edit;
                cache[cache_idx].mask = last_mask;
                cache[cache_idx].iface = last_iface;
                cache_idx++;
            } else if (lbit) {
                memcpy(cache + cache_idx, current->leaf + idx[0], sizeof(leaf_t));
                cache_idx++;
                idx[0]++;
            }
            first = 0;
        }
        vector = vector >> 1;
        leafvec = leafvec >> 1;
        bit_edit = bit_edit << 1;
    }

    leaf_t *new_leaf = malloc_leaf(cache_idx);
    memcpy(new_leaf, cache, sizeof(leaf_t) * cache_idx);
    free(current->leaf);
    current->leaf = new_leaf;
    counter_leaf += (cache_idx - idx[0]);
}

unsigned calc_space_poptrie() {
    return (counter_leaf * sizeof(leaf_t) + counter_internal * sizeof(internal_t));
}
