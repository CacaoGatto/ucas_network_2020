#include "tcp.h"
#include "tcp_sock.h"
#include "tcp_timer.h"

#include "log.h"
#include "ring_buffer.h"

#include <stdlib.h>
// update the snd_wnd of tcp_sock
//
// if the snd_wnd before updating is zero, notify tcp_sock_send (wait_send)
static inline void tcp_update_window(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u16 old_snd_wnd = tsk->snd_wnd;
	tsk->snd_wnd = cb->rwnd;
	if (old_snd_wnd == 0)
		wake_up(tsk->wait_send);
}

// update the snd_wnd safely: cb->ack should be between snd_una and snd_nxt
static inline void tcp_update_window_safe(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	if (less_or_equal_32b(tsk->snd_una, cb->ack) && less_or_equal_32b(cb->ack, tsk->snd_nxt))
		tcp_update_window(tsk, cb);
}

#ifndef max
#	define max(x,y) ((x)>(y) ? (x) : (y))
#endif

// check whether the sequence number of the incoming packet is in the receiving
// window
static inline int is_tcp_seq_valid(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u32 rcv_end = tsk->rcv_nxt + max(tsk->rcv_wnd, 1);
	if (less_than_32b(cb->seq, rcv_end) && less_or_equal_32b(tsk->rcv_nxt, cb->seq_end)) {
		return 1;
	}
	else {
		log(ERROR, "received packet with invalid seq, drop it.");
		return 0;
	}
}

// Process the incoming packet according to TCP state machine. 
void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	// TODO: implement %s please.\n, __FUNCTION__
	if (!tsk) {
	    printf("No corresponding tsk record!!!\n");
        return;
	}
	tsk->snd_una = cb->ack;
	tsk->rcv_nxt = cb->seq_end;
	switch (cb->flags) {
        case TCP_SYN:
            if (tsk->state == TCP_LISTEN) {
                struct tcp_sock *csk = alloc_tcp_sock();
                list_add_tail(&csk->list, &tsk->listen_queue);
                csk->sk_sip = cb->daddr;
                csk->sk_dip = cb->saddr;
                csk->sk_dport =cb->sport;
                csk->parent = tsk;
                csk->iss = tcp_new_iss();
                csk->snd_una = tsk->snd_una;
                csk->rcv_nxt = tsk->rcv_nxt;
                csk->snd_nxt = tsk->iss;
                struct sock_addr *skaddr = (struct sock_addr*)malloc(sizeof(struct sock_addr));
                skaddr->ip = htonl(cb->daddr);
                skaddr->port = htons(cb->dport);
                tcp_sock_bind(csk, skaddr);
                tcp_set_state(csk, TCP_SYN_RECV);
                tcp_hash(csk);
                tcp_send_control_packet(csk, TCP_SYN | TCP_ACK);
            } else printf("Recv SYN but current state is %d", tsk->state);
            break;
	    case (TCP_SYN | TCP_ACK):
	        if (tsk->state == TCP_SYN_SENT) {
	            wake_up(tsk->wait_connect);
	        }
            break;
        case TCP_ACK:
            switch (tsk->state) {
                case TCP_SYN_RECV:
                    tcp_sock_accept_enqueue(tsk);
                    wake_up(tsk->parent->wait_accept);
                    tcp_set_state(tsk, TCP_ESTABLISHED);
                    break;
                case TCP_FIN_WAIT_1:
                    tcp_set_state(tsk, TCP_FIN_WAIT_2);
                    break;
                case TCP_LAST_ACK:
                    tcp_set_state(tsk, TCP_CLOSED);
                    if (!tsk->parent) tcp_bind_unhash(tsk);
                    tcp_unhash(tsk);
                    break;
                default: printf("Unset state for ACK %d\n", tsk->state);
            }
            break;
        case (TCP_ACK | TCP_FIN):
            if (tsk->state == TCP_FIN_WAIT_1) {
                tcp_set_state(tsk, TCP_TIME_WAIT);
                tcp_send_control_packet(tsk, TCP_ACK);
                tcp_set_timewait_timer(tsk);
            } else printf("Recv ACK | FIN but current state is %d", tsk->state);
            break;
        case TCP_FIN:
            switch (tsk->state) {
                case TCP_ESTABLISHED:
                    tcp_set_state(tsk, TCP_LAST_ACK);
                    tcp_send_control_packet(tsk, TCP_ACK | TCP_FIN);
                    break;
                case TCP_FIN_WAIT_2:
                    tcp_set_state(tsk, TCP_TIME_WAIT);
                    tcp_send_control_packet(tsk, TCP_ACK);
                    tcp_set_timewait_timer(tsk);
                    break;
                default: printf("Unset state for FIN %d\n", tsk->state);
            }
            break;
        default: printf("Unset flag %d\n", cb->flags);
	}
}
