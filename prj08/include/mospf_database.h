#ifndef __MOSPF_DATABASE_H__
#define __MOSPF_DATABASE_H__

#include "base.h"
#include "list.h"

#include "mospf_proto.h"
#include "rtable.h"

#define MAX_NODES 10
#define NODE_ADD MAX_NODES
#define NODE_DELETE -MAX_NODES

typedef struct {
	struct list_head list;
	u32	rid;
	u16	seq;
	int nadv;
	int alive;
	int map2mtx; // =-1 to add, +100 to update, -100 to delete
	struct mospf_lsa *array;
} mospf_db_entry_t;

typedef struct {
    mospf_db_entry_t* dbEntry;
    int valid;
    int dist;
    u32 gw;
    iface_info_t* iface;
} mtx_map_t;

typedef struct {
    u32 network;
    u32 mask;
    int value;
} net_info_t;

extern struct list_head mospf_db;
extern pthread_mutex_t db_lock;
extern net_info_t path_map[MAX_NODES][MAX_NODES];

void init_mospf_db();

#endif
