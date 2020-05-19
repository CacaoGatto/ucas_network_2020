#include "ip.h"
#include "icmp.h"
#include "packet.h"
#include "arpcache.h"
#include "rtable.h"
#include "arp.h"

// #include "log.h"

#include <stdio.h>
#include <stdlib.h>

// initialize ip header 
void ip_init_hdr(struct iphdr *ip, u32 saddr, u32 daddr, u16 len, u8 proto)
{
	ip->version = 4;
	ip->ihl = 5;
	ip->tos = 0;
	ip->tot_len = htons(len);
	ip->id = rand();
	ip->frag_off = htons(IP_DF);
	ip->ttl = DEFAULT_TTL;
	ip->protocol = proto;
	ip->saddr = htonl(saddr);
	ip->daddr = htonl(daddr);
	ip->checksum = ip_checksum(ip);
}

// lookup in the routing table, to find the entry with the same and longest prefix.
// the input address is in host byte order
rt_entry_t *longest_prefix_match(u32 dst)
{
	// TODO: longest prefix match for the packet
	rt_entry_t *result = NULL, *entry;
	u32 result_mask = 0;
	list_for_each_entry(entry, &rtable, list) {
	    if (entry->mask > result_mask) {
	        u32 ip_mask = entry->mask & dst;
	        if (ip_mask == (entry->dest & entry->mask)) {
	            result = entry;
	            result_mask = entry->mask;
	        }
	    }
	}
	return result;
}

// send IP packet
//
// Different from forwarding packet, ip_send_packet sends packet generated by
// router itself. This function is used to send ICMP packets.
void ip_send_packet(char *packet, int len)
{
	// TODO: send ip packet
    struct iphdr* ih = packet_to_ip_hdr(packet);
    u32 dst = ntohl(ih->daddr);
    rt_entry_t* entry = longest_prefix_match(dst);
    u32 saddr = entry->iface->ip;
    ih->saddr = htonl(saddr);
    ih->checksum = ip_checksum(ih);
    u32 next_ip = entry->gw;
    if (!next_ip) next_ip = dst;
    iface_send_packet_by_arp(entry->iface, next_ip, packet, len);
}
