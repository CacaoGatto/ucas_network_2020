#include "mospf_daemon.h"
#include "mospf_proto.h"
#include "mospf_nbr.h"
#include "mospf_database.h"
#include "packet.h"

#include "ip.h"

#include "list.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
extern ustack_t *instance;

pthread_mutex_t mospf_lock;

void mospf_init() {
    pthread_mutex_init(&mospf_lock, NULL);

    instance->area_id = 0;
    // get the ip address of the first interface
    iface_info_t *iface = list_entry(instance->iface_list.next, iface_info_t, list);
    instance->router_id = iface->ip;
    instance->sequence_num = 0;
    instance->lsuint = MOSPF_DEFAULT_LSUINT;

    iface = NULL;
    list_for_each_entry(iface, &instance->iface_list, list) {
        iface->helloint = MOSPF_DEFAULT_HELLOINT;
        init_list_head(&iface->nbr_list);
    }

    init_mospf_db();
}

void *sending_mospf_hello_thread(void *param);

void *sending_mospf_lsu_thread(void *param);

void *checking_nbr_thread(void *param);

void *checking_database_thread(void *param);

void *dumping_database(void *param) {
    mospf_db_entry_t *db_entry = NULL, *db_entry_q = NULL;
    while (1) {
        printf("\n================= Dumping Database - Start =========================\n");
        if (!list_empty(&mospf_db)) {
            list_for_each_entry_safe(db_entry, db_entry_q, &mospf_db, list) {
                printf("RID       SUBNET    MASK           NBR_RID       MAP\n");
                for (int i = 0; i < db_entry->nadv; i++) {
                    printf(IP_FMT"  "IP_FMT"  "IP_FMT"  "IP_FMT"  %d\n",
                           HOST_IP_FMT_STR(db_entry->rid),
                           HOST_IP_FMT_STR(db_entry->array[i].network),
                           HOST_IP_FMT_STR(db_entry->array[i].mask),
                           HOST_IP_FMT_STR(db_entry->array[i].rid), db_entry->map2mtx);
                }
                printf("\n");
            } //exit(0);
        } else
            printf("Database is now empty.\n");
        printf("================= Dumping Database -  End =========================\n\n");
        sleep(5);
    }
    return NULL;
}

void mospf_run() {
    pthread_t hello, lsu, nbr, dbchk, debug;
    pthread_create(&hello, NULL, sending_mospf_hello_thread, NULL);
    pthread_create(&lsu, NULL, sending_mospf_lsu_thread, NULL);
    pthread_create(&nbr, NULL, checking_nbr_thread, NULL);
    pthread_create(&dbchk, NULL, checking_database_thread, NULL);
    //pthread_create(&debug, NULL, dumping_database, NULL);
}

void *sending_mospf_hello_thread(void *param) {
    // TODO: send mOSPF Hello message periodically.

    int length = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE;
    iface_info_t *iface;
    while (1) {
        pthread_mutex_lock(&mospf_lock);
        list_for_each_entry(iface, &instance->iface_list, list) {
            char *packet = (char *) malloc(length * sizeof(char));
            struct ether_header *ehdr = (struct ether_header *) packet;
            struct iphdr *ihdr = packet_to_ip_hdr(packet);
            struct mospf_hdr *mhdr = (struct mospf_hdr *) ((char *) ihdr + IP_BASE_HDR_SIZE);
            struct mospf_hello *mhello = (struct mospf_hello *) ((char *) mhdr + MOSPF_HDR_SIZE);

            mospf_init_hello(mhello, iface->mask);

            mospf_init_hdr(mhdr, MOSPF_TYPE_HELLO, MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE, instance->router_id, 0);
            mhdr->checksum = mospf_checksum(mhdr);

            ip_init_hdr(ihdr, iface->ip, MOSPF_ALLSPFRouters, length - ETHER_HDR_SIZE, IPPROTO_MOSPF);
            ihdr->checksum = ip_checksum(ihdr);

            ehdr->ether_dhost[0] = 0x01;
            ehdr->ether_dhost[2] = 0x5e;
            ehdr->ether_dhost[5] = 0x05;
            memcpy(ehdr->ether_shost, iface->mac, ETH_ALEN);
            ehdr->ether_type = htons(ETH_P_IP);

            iface_send_packet(iface, packet, length);
        }
        pthread_mutex_unlock(&mospf_lock);
        sleep(MOSPF_DEFAULT_HELLOINT);
    }

    return NULL;
}

void *checking_nbr_thread(void *param) {
    // TODO: neighbor list timeout operation.

    iface_info_t *iface;
    while (1) {
        pthread_mutex_lock(&mospf_lock);
        list_for_each_entry(iface, &instance->iface_list, list) {
            mospf_nbr_t *nbr, *q;
            //printf("%s : ", iface->name);
            list_for_each_entry_safe(nbr, q, &iface->nbr_list, list) {
                //printf("id : %d ; ip : %d ; mask : %d ", nbr->nbr_id, nbr->nbr_ip, nbr->nbr_mask);
                nbr->alive -= 3;
                if (nbr->alive <= 0) {
                    iface->num_nbr--;
                    list_delete_entry(&nbr->list);
                    free(nbr);
                    //printf("[delete]");
                }
                //printf("// ");
            }
            //printf("\n");
        }
        //printf("\n\n");
        pthread_mutex_unlock(&mospf_lock);
        sleep(3);
    }

    return NULL;
}

void handle_mospf_hello(iface_info_t *iface, const char *packet, int len) {
    // TODO: handle mOSPF Hello message.

    struct mospf_hdr *mhdr = (struct mospf_hdr *) (packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);
    u32 srid = ntohl(mhdr->rid);
    mospf_nbr_t *nbr;
    pthread_mutex_lock(&mospf_lock);
    list_for_each_entry(nbr, &iface->nbr_list, list) {
        if (nbr->nbr_id == srid) {
            nbr->alive = 3 * iface->helloint;
            pthread_mutex_unlock(&mospf_lock);
            return;
        }
    }

    struct iphdr *ihdr = packet_to_ip_hdr(packet);
    struct mospf_hello *mhello = (struct mospf_hello *) ((char *) mhdr + MOSPF_HDR_SIZE);
    nbr = (mospf_nbr_t *) malloc(sizeof(mospf_nbr_t));
    nbr->nbr_id = srid;
    nbr->nbr_ip = ntohl(ihdr->saddr);
    nbr->nbr_mask = ntohl(mhello->mask);
    nbr->alive = 3 * iface->helloint;
    list_add_tail(&nbr->list, &iface->nbr_list);
    iface->num_nbr++;
    pthread_mutex_unlock(&mospf_lock);
}

void *sending_mospf_lsu_thread(void *param) {
    // TODO: send mOSPF LSU message periodically.

    iface_info_t *iface;
    mospf_nbr_t *nbr;
    while (1) {
        //printf("send ckpt 1\n");
        u32 nadv = 0;
        pthread_mutex_lock(&mospf_lock);
        list_for_each_entry(iface, &instance->iface_list, list) {
            nadv += iface->num_nbr;
            if (!iface->num_nbr) nadv++;
        }
        u32 len_lsa = MOSPF_LSA_SIZE * nadv;
        struct mospf_lsa *lsa_all = (struct mospf_lsa *) malloc(len_lsa);
        struct mospf_lsa *temp = lsa_all;
        list_for_each_entry(iface, &instance->iface_list, list) {
            if (!iface->num_nbr) {
                temp->network = htonl(iface->ip & iface->mask);
                temp->mask = htonl(iface->mask);
                temp->rid = htonl(0);
                temp++;
            } else
                list_for_each_entry(nbr, &iface->nbr_list, list) {
                    temp->network = htonl(nbr->nbr_ip & nbr->nbr_mask);
                    temp->mask = htonl(nbr->nbr_mask);
                    temp->rid = htonl(nbr->nbr_id);
                    temp++;
                }
        }
        //printf("send ckpt 2\n");
        u32 len_pkt = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE + MOSPF_LSU_SIZE + len_lsa;
        list_for_each_entry(iface, &instance->iface_list, list) {
            if (iface->num_nbr)
                list_for_each_entry(nbr, &iface->nbr_list, list) {
                    char *packet = (char *) malloc(len_pkt);
                    //struct ether_header* ehdr = (struct ether_header*)packet;
                    struct iphdr *ihdr = packet_to_ip_hdr(packet);
                    struct mospf_hdr *mhdr = (struct mospf_hdr *) ((char *) ihdr + IP_BASE_HDR_SIZE);
                    struct mospf_lsu *mlsu = (struct mospf_lsu *) ((char *) mhdr + MOSPF_HDR_SIZE);
                    struct mospf_lsa *mlsa = (struct mospf_lsa *) ((char *) mlsu + MOSPF_LSU_SIZE);

                    memcpy(mlsa, lsa_all, len_lsa);

                    mospf_init_lsu(mlsu, nadv);

                    mospf_init_hdr(mhdr, MOSPF_TYPE_LSU, MOSPF_HDR_SIZE + MOSPF_LSU_SIZE
                                                         + len_lsa, instance->router_id, 0);
                    mhdr->checksum = mospf_checksum(mhdr);

                    ip_init_hdr(ihdr, iface->ip, nbr->nbr_ip, len_pkt - ETHER_HDR_SIZE, IPPROTO_MOSPF);
                    ihdr->checksum = ip_checksum(ihdr);

                    ip_send_packet(packet, len_pkt);
                }
        }
        //printf("send ckpt 3\n");

        free(lsa_all);
        instance->sequence_num++;
        pthread_mutex_unlock(&mospf_lock);
        sleep(instance->lsuint);
        //printf("send ckpt 4\n");
    }

    return NULL;
}

void handle_mospf_lsu(iface_info_t *iface, char *packet, int len) {
    // TODO: handle mOSPF LSU message.

    struct iphdr *ihdr = (struct iphdr *) packet_to_ip_hdr(packet);
    struct mospf_hdr *mhdr = (struct mospf_hdr *) ((char *) ihdr + IP_BASE_HDR_SIZE);
    struct mospf_lsu *mlsu = (struct mospf_lsu *) ((char *) mhdr + MOSPF_HDR_SIZE);
    if (ntohl(mhdr->rid) == instance->router_id) return;
    mospf_db_entry_t *entry;
    int new_entry = 1;
    pthread_mutex_lock(&db_lock);
    //printf("recv ckpt 1\n");
    list_for_each_entry(entry, &mospf_db, list) {
        if (ntohl(mhdr->rid) == entry->rid) {
            if (ntohs(mlsu->seq) <= entry->seq) {
                pthread_mutex_unlock(&db_lock);
                return;
            }
            new_entry = 0;
            break;
        }
    }
    //printf("recv ckpt 2\n");
    if (new_entry) {
        entry = (mospf_db_entry_t *) malloc(sizeof(mospf_db_entry_t));
        entry->rid = ntohl(mhdr->rid);
        //printf("recv ckpt 2.3\n");
        entry->map2mtx = NODE_ADD;
        entry->array = (struct mospf_lsa *) malloc(sizeof(struct mospf_lsa));
        list_add_tail(&entry->list, &mospf_db);
    }
    //printf("recv ckpt 2.5\n");
    entry->seq = ntohs(mlsu->seq);
    entry->nadv = ntohl(mlsu->nadv);
    entry->alive = MOSPF_DATABASE_TIMEOUT;
    u32 len_lsa = MOSPF_LSA_SIZE * entry->nadv;
    entry->array = (struct mospf_lsa *) realloc(entry->array, len_lsa);
    //printf("recv ckpt 2.8\n");
    struct mospf_lsa *mlsa = (struct mospf_lsa *) ((char *) mlsu + MOSPF_LSU_SIZE);
    struct mospf_lsa *dblsa = entry->array;
    //printf("recv ckpt 3\n");
    for (int i = 0; i < entry->nadv; ++i) {
        dblsa->rid = ntohl(mlsa->rid);
        dblsa->mask = ntohl(mlsa->mask);
        dblsa->network = ntohl(mlsa->network);
        dblsa++;
        mlsa++;
    }
    pthread_mutex_unlock(&db_lock);
    //printf("recv ckpt 4\n");
    if (--ihdr->ttl >= 0) {
        mospf_nbr_t *nbr;
        iface_info_t *ientry;
        pthread_mutex_lock(&mospf_lock);
        list_for_each_entry(ientry, &instance->iface_list, list) {
            if (ientry->index == iface->index) continue;
            ihdr->saddr = htonl(ientry->ip);
            if (ientry->num_nbr)
                list_for_each_entry(nbr, &ientry->nbr_list, list) {
                    char *cpacket = (char *) malloc(len);
                    memcpy(cpacket, packet, len);
                    struct iphdr *cihdr = packet_to_ip_hdr(cpacket);
                    cihdr->daddr = htonl(nbr->nbr_ip);
                    cihdr->checksum = ip_checksum(cihdr);
                    ip_send_packet(cpacket, len);
                }
        }
        pthread_mutex_unlock(&mospf_lock);
    }
}

void *checking_database_thread(void *param) {
    // TODO: link state database timeout operation.

    mospf_db_entry_t *dbEntry, *q;
    while (1) {
        pthread_mutex_lock(&db_lock);
        if (!list_empty(&mospf_db))
            list_for_each_entry_safe(dbEntry, q, &mospf_db, list) {
                dbEntry->alive -= 5;
                if (dbEntry->alive <= 0) {
                    dbEntry->map2mtx = NODE_DELETE - dbEntry->map2mtx;/*
                list_delete_entry(&dbEntry->list);
                for (int i = 0; i < dbEntry->nadv; ++i) {
                    free(dbEntry->array + i);
                }
                free(dbEntry);*/
                }
            }
        pthread_mutex_unlock(&db_lock);
        sleep(5);
    }

    return NULL;
}

void handle_mospf_packet(iface_info_t *iface, char *packet, int len) {
    struct iphdr *ip = (struct iphdr *) (packet + ETHER_HDR_SIZE);
    struct mospf_hdr *mospf = (struct mospf_hdr *) ((char *) ip + IP_HDR_SIZE(ip));

    if (mospf->version != MOSPF_VERSION) {
        log(ERROR, "received mospf packet with incorrect version (%d)", mospf->version);
        return;
    }
    if (mospf->checksum != mospf_checksum(mospf)) {
        log(ERROR, "received mospf packet with incorrect checksum");
        return;
    }
    if (ntohl(mospf->aid) != instance->area_id) {
        log(ERROR, "received mospf packet with incorrect area id");
        return;
    }

    switch (mospf->type) {
        case MOSPF_TYPE_HELLO:
            handle_mospf_hello(iface, packet, len);
            break;
        case MOSPF_TYPE_LSU:
            handle_mospf_lsu(iface, packet, len);
            break;
        default:
            log(ERROR, "received mospf packet with unknown type (%d).", mospf->type);
            break;
    }
}

#pragma clang diagnostic pop