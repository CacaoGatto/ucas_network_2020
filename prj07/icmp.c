#include "icmp.h"
#include "ip.h"
#include "rtable.h"
#include "arp.h"
#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// send icmp packet
void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code)
{
	// TODO: malloc and send icmp packet.
    //struct ether_header *in_eh = (struct ether_header *)in_pkt;
    struct iphdr *in_ih = packet_to_ip_hdr(in_pkt);
	int out_len;
	if (type == ICMP_ECHOREPLY) out_len = len - IP_HDR_SIZE(in_ih) + IP_BASE_HDR_SIZE;
	else out_len = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + ICMP_HDR_SIZE + IP_HDR_SIZE(in_ih) + ICMP_COPIED_DATA_LEN;

	char* out_pkt = (char*)malloc(out_len * sizeof(char));

	//struct ether_header *out_eh = (struct ether_header *)out_pkt;
	//memcpy(out_eh->ether_dhost, in_eh->ether_shost, ETH_ALEN);
	//out_eh->ether_type = htons(ETH_P_IP);

	struct iphdr *out_ih = packet_to_ip_hdr(out_pkt);
	ip_init_hdr(out_ih, 0, ntohl(in_ih->saddr), out_len - ETHER_HDR_SIZE, 1);

	struct icmphdr *out_ich = (struct icmphdr*)(IP_DATA(out_ih));
	out_ich->type = type;
	out_ich->code = code;
    char *out_rest = (char*)out_ich + ICMP_HDR_SIZE;
    if (type == ICMP_ECHOREPLY) {
        int rest_offset = ETHER_HDR_SIZE + IP_HDR_SIZE(in_ih) + 4;
        memcpy(out_rest - 4, in_pkt + rest_offset, len - rest_offset);
    } else {
        memset(&out_ich->u, 0, 4);
        memcpy(out_rest, in_ih, IP_HDR_SIZE(in_ih) + 8);
    }
	out_ich->checksum = icmp_checksum(out_ich, out_len - ETHER_HDR_SIZE - IP_BASE_HDR_SIZE);
    ip_send_packet(out_pkt, out_len);
}
