#include "nat.h"
#include "ip.h"
#include "icmp.h"
#include "tcp.h"
#include "rtable.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static struct nat_table nat;

// get the interface from iface name
static iface_info_t *if_name_to_iface(const char *if_name) {
    iface_info_t *iface = NULL;
    list_for_each_entry(iface, &instance->iface_list, list) {
        if (strcmp(iface->name, if_name) == 0)
            return iface;
    }

    log(ERROR, "Could not find the desired interface according to if_name '%s'", if_name);
    return NULL;
}

// determine the direction of the packet, DIR_IN / DIR_OUT / DIR_INVALID
static int get_packet_direction(char *packet) {
    // TODO: determine the direction of this packet.
    struct iphdr *iphdr = packet_to_ip_hdr(packet);
    u32 daddr = ntohl(iphdr->daddr);
    u32 saddr = ntohl(iphdr->saddr);
    u32 d_out = longest_prefix_match(daddr)->iface->ip;
    u32 s_out = longest_prefix_match(saddr)->iface->ip;
    u32 int_ip = nat.internal_iface->ip;
    u32 ext_ip = nat.external_iface->ip;
    if (d_out == ext_ip && s_out == int_ip) return DIR_OUT;
    else if (s_out == ext_ip && daddr == ext_ip) return DIR_IN;
    else return DIR_INVALID;
}

static u8 remote2hash(u32 addr, u16 port) {
    char buf[6];
    memset(buf, 0, 6 * sizeof(char));
    memcpy(buf, &addr, 4 * sizeof(char));
    memcpy(buf + 4, &port, 2 * sizeof(char));
    return hash8(buf, 6);
}

// do translation for the packet: replace the ip/port, recalculate ip & tcp
// checksum, update the statistics of the tcp connection
void do_translation(iface_info_t *iface, char *packet, int len, int dir) {
    // TODO: do translation for this packet.
    struct iphdr *iphdr = packet_to_ip_hdr(packet);
    u32 daddr = ntohl(iphdr->daddr);
    u32 saddr = ntohl(iphdr->saddr);
    u32 raddr = (dir == DIR_IN) ? saddr : daddr;
    struct tcphdr *tcphdr = packet_to_tcp_hdr(packet);
    u16 sport = ntohs(tcphdr->sport);
    u16 dport = ntohs(tcphdr->dport);
    u16 rport = (dir == DIR_IN) ? sport : dport;
    u8 idx = remote2hash(raddr, rport);
    struct list_head *head = &nat.nat_mapping_list[idx];
    struct nat_mapping *entry;
    pthread_mutex_lock(&nat.lock);
    list_for_each_entry(entry, head, list) {
        if (raddr != entry->remote_ip || rport != entry->remote_port) continue;
        if (dir == DIR_IN) {
            if (daddr != entry->external_ip || dport != entry->external_port) continue;
            iphdr->daddr = htonl(entry->internal_ip);
            tcphdr->dport = htons(entry->internal_port);
            entry->conn.external_fin = ((tcphdr->flags & TCP_FIN) != 0);
            entry->conn.external_seq_end = tcp_seq_end(iphdr, tcphdr);
            if (tcphdr->flags & TCP_ACK) entry->conn.external_ack = tcphdr->ack;
        } else {
            if (saddr != entry->internal_ip || sport != entry->internal_port) continue;
            iphdr->saddr = htonl(entry->external_ip);
            tcphdr->sport = htons(entry->external_port);
            entry->conn.internal_fin = ((tcphdr->flags & TCP_FIN) != 0);
            entry->conn.internal_seq_end = tcp_seq_end(iphdr, tcphdr);
            if (tcphdr->flags & TCP_ACK) entry->conn.internal_ack = tcphdr->ack;
        }
        entry->update_time = time(NULL);
        pthread_mutex_unlock(&nat.lock);
        tcphdr->checksum = tcp_checksum(iphdr, tcphdr);
        iphdr->checksum = ip_checksum(iphdr);
        ip_send_packet(packet, len);
        return;
    }
    if ((tcphdr->flags & TCP_SYN) == 0) {
        printf("Invalid packet!\n");
        icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
        free(packet);
        pthread_mutex_unlock(&nat.lock);
        return;
    }
    if (dir == DIR_OUT) {
        u16 pid;
        for (pid = NAT_PORT_MIN; pid <= NAT_PORT_MAX; ++pid) {
            if (!nat.assigned_ports[pid]) {
                struct nat_mapping *new_entry = (struct nat_mapping *) malloc(sizeof(struct nat_mapping));
                list_add_tail(&new_entry->list, head);

                new_entry->remote_ip = raddr;
                new_entry->remote_port = rport;
                new_entry->external_ip = nat.external_iface->ip;
                new_entry->external_port = pid;
                new_entry->internal_ip = saddr;
                new_entry->internal_port = sport;

                new_entry->conn.internal_fin = ((tcphdr->flags & TCP_FIN) != 0);
                new_entry->conn.internal_seq_end = tcp_seq_end(iphdr, tcphdr);
                if (tcphdr->flags & TCP_ACK) new_entry->conn.internal_ack = tcphdr->ack;

                new_entry->update_time = time(NULL);
                pthread_mutex_unlock(&nat.lock);

                iphdr->saddr = htonl(new_entry->external_ip);
                tcphdr->sport = htons(new_entry->external_port);
                tcphdr->checksum = tcp_checksum(iphdr, tcphdr);
                iphdr->checksum = ip_checksum(iphdr);
                ip_send_packet(packet, len);
                return;
            }
        }
    } else {
        struct dnat_rule *rule;
        list_for_each_entry(rule, &nat.rules, list) {
            if (daddr == rule->external_ip && dport == rule->external_port) {
                struct nat_mapping *new_entry = (struct nat_mapping *) malloc(sizeof(struct nat_mapping));
                list_add_tail(&new_entry->list, head);

                new_entry->remote_ip = raddr;
                new_entry->remote_port = rport;
                new_entry->external_ip = rule->external_ip;
                new_entry->external_port = rule->external_port;
                new_entry->internal_ip = rule->internal_ip;
                new_entry->internal_port = rule->internal_port;

                new_entry->conn.external_fin = ((tcphdr->flags & TCP_FIN) != 0);
                new_entry->conn.external_seq_end = tcp_seq_end(iphdr, tcphdr);
                if (tcphdr->flags & TCP_ACK) new_entry->conn.external_ack = tcphdr->ack;

                new_entry->update_time = time(NULL);
                pthread_mutex_unlock(&nat.lock);

                iphdr->daddr = htonl(rule->internal_ip);
                tcphdr->dport = htons(rule->internal_port);
                tcphdr->checksum = tcp_checksum(iphdr, tcphdr);
                iphdr->checksum = ip_checksum(iphdr);
                ip_send_packet(packet, len);
                return;
            }
        }
    }
    printf("No available port!\n");
    icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
    free(packet);
}

void nat_translate_packet(iface_info_t *iface, char *packet, int len) {
    int dir = get_packet_direction(packet);
    if (dir == DIR_INVALID) {
        log(ERROR, "invalid packet direction, drop it.");
        icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
        free(packet);
        return;
    }

    struct iphdr *ip = packet_to_ip_hdr(packet);
    if (ip->protocol != IPPROTO_TCP) {
        log(ERROR, "received non-TCP packet (0x%0hhx), drop it", ip->protocol);
        free(packet);
        return;
    }

    do_translation(iface, packet, len, dir);
}

// check whether the flow is finished according to FIN bit and sequence number
// XXX: seq_end is calculated by `tcp_seq_end` in tcp.h
static int is_flow_finished(struct nat_connection *conn) {
    return (conn->internal_fin && conn->external_fin) && \
            (conn->internal_ack >= conn->external_seq_end) && \
            (conn->external_ack >= conn->internal_seq_end);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

// nat timeout thread: find the finished flows, remove them and free port
// resource
void *nat_timeout() {
    while (1) {
        // TODO: sweep finished flows periodically.
        pthread_mutex_lock(&nat.lock);
        struct nat_mapping *entry, *q;
        time_t now = time(NULL);
        for (int i = 0; i < HASH_8BITS; ++i) {
            list_for_each_entry_safe(entry, q, &nat.nat_mapping_list[i], list) {
                if ((now - entry->update_time >= TCP_ESTABLISHED_TIMEOUT) || is_flow_finished(&entry->conn)) {
                    nat.assigned_ports[entry->external_port] = 0;
                    list_delete_entry(&entry->list);
                    free(entry);
                }
            }
        }
        pthread_mutex_unlock(&nat.lock);
        sleep(1);
    }

    return NULL;
}

#pragma clang diagnostic pop

int parse_config(const char *filename) {
    // TODO: parse config file, including i-iface, e-iface (and dnat-rules if existing).
    char line[256];
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Open fail errno = %d. reason = %s \n", errno, strerror(errno));
        char buf[1024];
        printf("Working path : %s\n", getcwd(buf, 1024));
    }
    char type[128], name[128], exter[64], inter[64];
    while (!feof(fp) && !ferror(fp)) {
        strcpy(line, "\n");
        fgets(line, sizeof(line), fp);
        if (line[0] == '\n') break;
        sscanf(line, "%s %s", type, name);
        type[14] = '\0';
        if (strcmp(type, "internal-iface") == 0) {
            printf("[Internal] Loading iface item : %s .\n", name);
            nat.internal_iface = if_name_to_iface(name);
        } else if (strcmp(type, "external-iface") == 0) {
            printf("[External] Loading iface item : %s .\n", name);
            nat.external_iface = if_name_to_iface(name);
        } else printf("[Unknown] Loading failed : %s .\n", type);
    }
    u32 ip4, ip3, ip2, ip1, ip;
    u16 port;
    while (!feof(fp) && !ferror(fp)) {
        strcpy(line, "\n");
        fgets(line, sizeof(line), fp);
        if (line[0] == '\n') break;
        sscanf(line, "%s %s %s %s", type, exter, name, inter);
        type[10] = '\0';
        if (strcmp(type, "dnat-rules") == 0) {
            printf("[Dnat] Loading rule item : %s to %s.\n", exter, inter);
            struct dnat_rule *rule = (struct dnat_rule*)malloc(sizeof(struct dnat_rule));
            list_add_tail(&rule->list, &nat.rules);

            sscanf(exter, "%[^:]:%hu", name, &port);
            sscanf(name, "%u.%u.%u.%u", &ip4, &ip3, &ip2, &ip1);
            ip = (ip4 << 24) | (ip3 << 16) | (ip2 << 8) | (ip1);
            rule->external_ip = ip;
            rule->external_port = port;
            printf("   |---[External] ip : %08x ; port : %hu\n", ip, port);

            sscanf(inter, "%[^:]:%hu", name, &port);
            sscanf(name, "%u.%u.%u.%u", &ip4, &ip3, &ip2, &ip1);
            ip = (ip4 << 24) | (ip3 << 16) | (ip2 << 8) | (ip1);
            rule->internal_ip = ip;
            rule->internal_port = port;
            printf("   |---[Internal] ip : %08x ; port : %hu\n", ip, port);
        }
        else printf("[Unknown] Loading failed : %s .\n", type);
    }
    return 0;
}

// initialize
void nat_init(const char *config_file) {
    memset(&nat, 0, sizeof(nat));

    for (int i = 0; i < HASH_8BITS; i++)
        init_list_head(&nat.nat_mapping_list[i]);

    init_list_head(&nat.rules);

    // seems unnecessary
    memset(nat.assigned_ports, 0, sizeof(nat.assigned_ports));

    parse_config(config_file);

    pthread_mutex_init(&nat.lock, NULL);

    pthread_create(&nat.thread, NULL, nat_timeout, NULL);
}

void nat_exit() {
    // TODO: release all resources allocated.
    pthread_mutex_lock(&nat.lock);
    struct nat_mapping *entry, *q;
    for (int i = 0; i < HASH_8BITS; ++i) {
        list_for_each_entry_safe(entry, q, &nat.nat_mapping_list[i], list) {
            list_delete_entry(&entry->list);
            free(entry);
        }
    }
    pthread_kill(nat.thread, SIGTERM);
    pthread_mutex_unlock(&nat.lock);
}
