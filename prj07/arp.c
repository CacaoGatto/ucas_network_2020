#include "arp.h"
#include "base.h"
#include "types.h"
#include "packet.h"
#include "ether.h"
#include "arpcache.h"
// #include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "log.h"

static void arp_init_hdr(struct ether_arp* arp, iface_info_t *iface, u16 op, u32 dst_ip) {
    arp->arp_hrd = htons(0x1);
    arp->arp_pro = htons(0x800);
    arp->arp_hln = 6;
    arp->arp_pln = 4;
    arp->arp_op = htons(op);
    arp->arp_spa = htonl(iface->ip);
    memcpy(arp->arp_sha, iface->mac, ETH_ALEN);
    arp->arp_tpa = htonl(dst_ip);
}

// send an arp request: encapsulate an arp request packet, send it out through
// iface_send_packet
void arp_send_request(iface_info_t *iface, u32 dst_ip)
{
	// TODO: send arp request when lookup failed in arpcache.
    int len = ETHER_HDR_SIZE + sizeof(struct ether_arp);
    char* packet = (char*)malloc(len * sizeof(char));
    struct ether_header *eh = (struct ether_header *)packet;
    memcpy(eh->ether_shost, iface->mac, ETH_ALEN);
    memset(eh->ether_dhost, -1, ETH_ALEN);
    eh->ether_type = htons(ETH_P_ARP);
    struct ether_arp* ah = (struct ether_arp*)(packet + ETHER_HDR_SIZE);
    arp_init_hdr(ah, iface, ARPOP_REQUEST, dst_ip);
    memset(ah->arp_tha, 0, ETH_ALEN);
    iface_send_packet(iface, packet, len);
}

// send an arp reply packet: encapsulate an arp reply packet, send it out
// through iface_send_packet
void arp_send_reply(iface_info_t *iface, struct ether_arp *req_hdr)
{
	// TODO: send arp reply when receiving arp request.
	int len = ETHER_HDR_SIZE + sizeof(struct ether_arp);
	char* packet = (char*)malloc(len * sizeof(char));
    struct ether_header *eh = (struct ether_header *)packet;
    memcpy(eh->ether_shost, iface->mac, ETH_ALEN);
    memcpy(eh->ether_dhost, req_hdr->arp_sha, ETH_ALEN);
    eh->ether_type = htons(ETH_P_ARP);
    struct ether_arp* ah = (struct ether_arp*)(packet + ETHER_HDR_SIZE);
    arp_init_hdr(ah, iface, ARPOP_REPLY, ntohl(req_hdr->arp_spa));
    memcpy(ah->arp_tha, req_hdr->arp_sha, ETH_ALEN);
    iface_send_packet(iface, packet, len);
}

void handle_arp_packet(iface_info_t *iface, char *packet, int len)
{
	// TODO: process arp packet: arp request & arp reply.
    struct ether_arp* ah = (struct ether_arp*)(packet + ETHER_HDR_SIZE);
    if (ntohl(ah->arp_tpa) != iface->ip) free(packet);
    else {
        if (ntohs(ah->arp_op) == ARPOP_REQUEST) arp_send_reply(iface, ah);
        else if (ntohs(ah->arp_op) == ARPOP_REPLY) arpcache_insert(ntohl(ah->arp_spa), ah->arp_sha);
        else free(packet);
    }
}

// send (IP) packet through arpcache lookup 
//
// Lookup the mac address of dst_ip in arpcache. If it is found, fill the
// ethernet header and emit the packet by iface_send_packet, otherwise, pending 
// this packet into arpcache, and send arp request.
void iface_send_packet_by_arp(iface_info_t *iface, u32 dst_ip, char *packet, int len)
{
	struct ether_header *eh = (struct ether_header *)packet;
	memcpy(eh->ether_shost, iface->mac, ETH_ALEN);
	eh->ether_type = htons(ETH_P_IP);

    FILE *temp = fopen("debug_arp.txt", "a+");
    char line = '\n';
    fwrite(&line, sizeof(char), 1, temp);
    fwrite(packet, sizeof(char), len, temp);
    fclose(temp);

	u8 dst_mac[ETH_ALEN];
	int found = arpcache_lookup(dst_ip, dst_mac);
	if (found) {
		// log(DEBUG, "found the mac of %x, send this packet", dst_ip);
		memcpy(eh->ether_dhost, dst_mac, ETH_ALEN);
		iface_send_packet(iface, packet, len);
	}
	else {
		// log(DEBUG, "lookup %x failed, pend this packet", dst_ip);
		arpcache_append_packet(iface, dst_ip, packet, len);
	}
}
