#include "mospf_database.h"
#include "ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <mospf_nbr.h>

struct list_head mospf_db;
pthread_mutex_t db_lock;
net_info_t path_map[MAX_NODES][MAX_NODES];
mtx_map_t record[MAX_NODES];
int sup = 0, start = 0;

void *setting_rt_list_thread(void *param);

void init_mospf_db() {
    init_list_head(&mospf_db);
    for (int i = 0; i < MAX_NODES; ++i) {
        record[i].valid = 0;
    }
    rt_entry_t *rtEntry;
    list_for_each_entry(rtEntry, &rtable, list) {
        start++;
    }
    pthread_mutex_init(&db_lock, NULL);
    pthread_t rt;
    pthread_create(&rt, NULL, setting_rt_list_thread, NULL);
}

void init_matrix() {
    for (int i = 0; i < MAX_NODES; ++i) {
        for (int j = 0; j < MAX_NODES; ++j) {
            path_map[i][j].value = 0;
        }
        record[i].dist = 0;
    }
}

void edit_mtx(int line) {
    mospf_db_entry_t *dbEntry = record[line].dbEntry;
    struct mospf_lsa *lsa_item;
    pthread_mutex_lock(&db_lock);
    int i;
    for (i = 0; i < dbEntry->nadv; ++i) {
        lsa_item = dbEntry->array + i;
        for (int j = 0; j < sup; ++j) {
            if (record[j].valid && (record[j].dbEntry->rid == lsa_item->rid)) {
                path_map[line][j].value = 1;
                path_map[line][j].network = lsa_item->network;
                path_map[line][j].mask = lsa_item->mask;
                //printf("  "IP_FMT, HOST_IP_FMT_STR(lsa_item->rid));
            } else path_map[line][j].value = 0;
        }
    }
    //printf("     %d\n", i);
    pthread_mutex_unlock(&db_lock);
}

void renew_matrix() {
    mospf_db_entry_t *dbEntry, *q;
    int count = 0;
    pthread_mutex_lock(&db_lock);
    if (!list_empty(&mospf_db))
        list_for_each_entry_safe(dbEntry, q, &mospf_db, list) {
            if (dbEntry->map2mtx <= NODE_DELETE) {
                int num_delete = NODE_DELETE - dbEntry->map2mtx;
                record[num_delete].valid = 0;
                list_delete_entry(&dbEntry->list);
                free(dbEntry->array);
                free(dbEntry);
                count--;
            } else if (dbEntry->map2mtx == NODE_ADD) {
                for (int i = 0; i < MAX_NODES; i++) {
                    if (record[i].valid == 0) {
                        record[i].valid = 1;
                        record[i].dbEntry = dbEntry;
                        dbEntry->map2mtx = i;
                        if (i > sup) sup = i;
                        break;
                    }
                }
            }
            count++;
        }
    pthread_mutex_unlock(&db_lock);
    init_matrix();
    int i = 0;
    while (count) {
        if (!record[i++].valid) continue;
        count--;
        //printf("editting line %d ||", i-1);
        edit_mtx(i - 1);
    }
    sup = i;
}

rt_entry_t *chk_init(u32 subnet) {
    rt_entry_t *item, *q;
    int check = 0;
    list_for_each_entry_safe(item, q, &rtable, list) {
        if (check++ >= start) break;
        if (subnet != (item->mask & item->dest)) continue;
        return item;
    }
    return NULL;
}

void calc_path() {
    iface_info_t *iface;
    mospf_nbr_t *nbr;
    rt_entry_t *item, *q;
    int check = 0;
    list_for_each_entry_safe(item, q, &rtable, list) {
        if (check++ < start) continue;
        list_delete_entry(&item->list);
        free(item);
    }
    int farther = 0;
    pthread_mutex_lock(&mospf_lock);
    list_for_each_entry(iface, &instance->iface_list, list) {
        list_for_each_entry(nbr, &iface->nbr_list, list) {
            for (int i = 0; i < sup; ++i) {
                if (record[i].valid && (record[i].dbEntry->rid == nbr->nbr_id)) {
                    record[i].dist = 1;
                    record[i].gw = nbr->nbr_ip;
                    record[i].iface = iface;
                    rt_entry_t *test = chk_init(nbr->nbr_ip & nbr->nbr_mask);
                    if (test) {
                        farther = 1;
                        test->gw = nbr->nbr_ip;
                        break;
                    }
                    rt_entry_t *temp = new_rt_entry(nbr->nbr_ip & nbr->nbr_mask, nbr->nbr_mask,
                                                    nbr->nbr_ip, iface);
                    add_rt_entry(temp);
                    farther = 1;
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&mospf_lock);
    int dist = 0;
    while (farther) {
        farther = 0;
        dist++;
        for (int i = 0; i < sup; ++i) {
            if (record[i].dist == dist) {
                for (int j = 0; j < sup; ++j) {
                    if (path_map[i][j].value == 1 && record[j].dist == 0) {
                        record[j].dist = dist + 1;
                        record[j].gw = record[i].gw;
                        record[j].iface = record[i].iface;
                        rt_entry_t *temp = new_rt_entry(path_map[i][j].network & path_map[i][j].mask,
                                                        path_map[i][j].mask, record[j].gw, record[j].iface);
                        add_rt_entry(temp);
                        farther = 1;
                        break;
                    }
                }
                mospf_db_entry_t *dbEntry = record[i].dbEntry;
                struct mospf_lsa *lsa_item;
                pthread_mutex_lock(&db_lock);
                //printf("line %d :", i);
                for (int k = 0; k < dbEntry->nadv; ++k) {
                    lsa_item = dbEntry->array + k;
                    if (lsa_item->rid == instance->router_id) {
                        //printf(IP_FMT" get from router\n", HOST_IP_FMT_STR(lsa_item->rid));
                        continue;
                    }
                    int host = 1;
                    for (int l = 0; l < sup; ++l) {
                        if (record[l].valid && (record[l].dbEntry->rid == lsa_item->rid)) {
                            host = 0;
                            //printf(IP_FMT" get from %d\n", HOST_IP_FMT_STR(lsa_item->rid), l);
                            break;
                        }
                    }
                    if (host) {
                        //printf(IP_FMT" get from host\n", HOST_IP_FMT_STR(lsa_item->rid));
                        rt_entry_t *temp = new_rt_entry(lsa_item->mask & lsa_item->network, lsa_item->mask,
                                                        record[i].gw, record[i].iface);
                        add_rt_entry(temp);
                    }
                }
                pthread_mutex_unlock(&db_lock);
            }
        }
    }
}

void print_matrix() {
    for (int i = 0; i < MAX_NODES; ++i) {
        for (int j = 0; j < MAX_NODES; ++j) {
            printf("%d  ", path_map[i][j].value);
        }
        printf("\n");
    }
    printf("RID       dist    valid\n");
    u32 zero = 0;
    for (int i = 0; i < MAX_NODES; ++i) {
        if (record[i].valid)
            printf(IP_FMT"  %d    %d\n",
                   HOST_IP_FMT_STR(record[i].dbEntry->rid),
                   record[i].dist,
                   record[i].valid);
        else
            printf(IP_FMT"  %d    %d\n",
                   HOST_IP_FMT_STR(zero),
                   record[i].dist,
                   record[i].valid);
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void *setting_rt_list_thread(void *param) {
    while (1) {
        renew_matrix();
        calc_path();
        //print_matrix();
        print_rtable();
        sleep(5);
    }
    return NULL;
}

#pragma clang diagnostic pop
