#include "ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <icmp.h>
#include <rtable.h>
#include <arp.h>

// handle ip packet
//
// If the packet is ICMP echo request and the destination IP address is equal to
// the IP address of the iface, send ICMP echo reply; otherwise, forward the
// packet.
void handle_ip_packet(iface_info_t *iface, char *packet, int len)
{
	// TODO: handle ip packet

    struct iphdr* hdr = packet_to_ip_hdr(packet);
    u32 pip = iface->ip, dip = ntohl(hdr->daddr);
    if (pip == dip) {
        struct icmphdr* echo = (struct icmphdr*)(IP_DATA(hdr));
        if (echo->type == ICMP_ECHOREQUEST)
            icmp_send_packet(packet, len, ICMP_ECHOREPLY, 0);
        free(packet);
    } else {
        rt_entry_t* entry = longest_prefix_match(dip);
        if (!entry) {
            icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_NET_UNREACH);
            free(packet);
        } else {
            (hdr->ttl)--;
            if (hdr->ttl == 0) {
                icmp_send_packet(packet, len, ICMP_TIME_EXCEEDED, ICMP_EXC_TTL);
                free(packet);
            } else {
                u32 dst_ip = entry->gw;
                if (!dst_ip) dst_ip = dip;
                hdr->checksum = ip_checksum(hdr);
                iface_send_packet_by_arp(entry->iface, dst_ip, packet, len);
            }
        }
    }
}
