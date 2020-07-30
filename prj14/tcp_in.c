#include "tcp.h"
#include "tcp_sock.h"
#include "tcp_timer.h"

#include "log.h"
#include "ring_buffer.h"

#include <stdlib.h>

void update_log(struct tcp_sock *tsk) {
#ifdef LOG_BY_TIME
    return;
#endif
    struct timeval current;
    gettimeofday(&current, NULL);
    long duration = 1000000 * ( current.tv_sec - start.tv_sec ) + current.tv_usec - start.tv_usec;
    char line[100];
    sprintf(line, "%ld\t%d\n", duration, tsk->cwnd);
    fwrite(line, sizeof(char), strlen(line), record_file);
}

static inline void tcp_cwnd_inc(struct tcp_sock *tsk) {
    if (tsk->cgt_state != OPEN) return;
    if (tsk->cwnd < tsk->ssthresh) {
        tsk->cwnd += 1;
    }
    else {
        tsk->cwnd_unit += 1;
        if (tsk->cwnd_unit >= tsk->cwnd) {
            tsk->cwnd_unit = 0;
            tsk->cwnd += 1;
        }
    }
}

// update the snd_wnd of tcp_sock
//
// if the snd_wnd before updating is zero, notify tcp_sock_send (wait_send)
static inline void tcp_update_window(struct tcp_sock *tsk, struct tcp_cb *cb) {
    u16 old_snd_wnd = tsk->snd_wnd;
    tsk->adv_wnd = cb->rwnd;
    tsk->snd_wnd = min(tsk->adv_wnd, tsk->cwnd * (MSS));
    if (old_snd_wnd == 0)
        wake_up(tsk->wait_send);
}

// update the snd_wnd safely: cb->ack should be between snd_una and snd_nxt
static inline void tcp_update_window_safe(struct tcp_sock *tsk, struct tcp_cb *cb) {
    if (less_or_equal_32b(tsk->snd_una, cb->ack) && less_or_equal_32b(cb->ack, tsk->snd_nxt))
        tcp_update_window(tsk, cb);
}

#ifndef max
#	define max(x, y) ((x)>(y) ? (x) : (y))
#endif

// check whether the sequence number of the incoming packet is in the receiving
// window
static inline int is_tcp_seq_valid(struct tcp_sock *tsk, struct tcp_cb *cb) {
    u32 rcv_end = tsk->rcv_nxt + max(tsk->rcv_wnd, 1);
    if (less_than_32b(cb->seq, rcv_end) && less_or_equal_32b(tsk->rcv_nxt, cb->seq_end)) {
        return 1;
    } else {
        log(ERROR, "received packet with invalid seq, drop it.");
        return 0;
    }
}

static void write_ofo_buffer(struct tcp_sock *tsk, struct tcp_cb *cb) {
    struct ofo_buffer *buf = (struct ofo_buffer *) malloc(sizeof(struct ofo_buffer));
    buf->tsk = tsk;
    buf->seq = cb->seq;
    buf->seq_end = cb->seq_end;
    buf->pl_len = cb->pl_len;
    buf->payload = (char *) malloc(buf->pl_len);
    memcpy(buf->payload, cb->payload, buf->pl_len);
    struct ofo_buffer head_ext;
    head_ext.list = tsk->rcv_ofo_buf;
    int insert = 0;
    struct ofo_buffer *pos, *last = &head_ext;
    list_for_each_entry(pos, &tsk->rcv_ofo_buf, list) {
        if (cb->seq > pos->seq) {
            last = pos;
            continue;
        } else if (cb->seq == pos->seq) return;
        list_insert(&buf->list, &last->list, &pos->list);
        insert = 1;
        break;
    }
    if (!insert) list_add_tail(&buf->list, &tsk->rcv_ofo_buf);
}

// Process the incoming packet according to TCP state machine. 
void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet) {
    // TODO: implement %s please.\n, __FUNCTION__
    if (!tsk) {
        printf("No corresponding tsk record!!!\n");
        return;
    }

    if (((cb->flags) & TCP_PSH) == 0) tsk->rcv_nxt = cb->seq_end;

    if ((cb->flags) & TCP_ACK) {
        if (cb->ack == tsk->snd_una) {
            tsk->rep_ack++;
            switch (tsk->cgt_state) {
                case OPEN:
                    if (tsk->rep_ack > 2) {
                        tsk->cgt_state = RCVR;
                        tsk->ssthresh = (tsk->cwnd + 1) / 2;
                        tsk->recovery_point = tsk->snd_nxt;
                        struct send_buffer *buf = NULL;
                        list_for_each_entry(buf, &tsk->send_buf, list) break;
                        if (!list_empty(&tsk->send_buf)) {
                            char *temp = (char *) malloc(buf->len * sizeof(char));
                            memcpy(temp, buf->packet, buf->len);
                            ip_send_packet(temp, buf->len);
                        }
                    } else break;
                case RCVR:
                    if (tsk->rep_ack > 1) {
                        tsk->rep_ack -= 2;
                        tsk->cwnd -= 1;
                        if (tsk->cwnd < 1) tsk->cwnd = 1;
                    }
                    break;
                default:
                    break;
            }
        } else {
            tsk->rep_ack = 0;
            struct send_buffer *buf, *q;
            list_for_each_entry_safe(buf, q, &tsk->send_buf, list) {
                if (buf->seq_end > cb->ack) break;
                else {
                    tsk->snd_una = buf->seq_end;
                    tcp_pop_sendbuf(tsk, buf);
                    tcp_cwnd_inc(tsk);
                }
            }
            switch (tsk->cgt_state) {
                case RCVR:
                    if (less_or_equal_32b(tsk->recovery_point, cb->ack)) {
                        tsk->cgt_state = OPEN;
                    } else {
                        char *temp = (char *) malloc(buf->len * sizeof(char));
                        memcpy(temp, buf->packet, buf->len);
                        ip_send_packet(temp, buf->len);
                    }
                    break;
                case LOSS:
                    if (less_or_equal_32b(tsk->recovery_point, cb->ack)) {
                        tsk->cgt_state = OPEN;
                    }
                    break;
                default:
                    break;
            }
        }
        update_log(tsk);
        tcp_update_window_safe(tsk, cb);
    }

    switch (cb->flags) {
        case TCP_SYN:
            switch (tsk->state) {
                case TCP_LISTEN:;
                    struct tcp_sock *csk = alloc_tcp_sock();
                    list_add_tail(&csk->list, &tsk->listen_queue);
                    csk->sk_sip = cb->daddr;
                    csk->sk_dip = cb->saddr;
                    csk->sk_dport = cb->sport;
                    csk->sk_sport = cb->dport;
                    csk->parent = tsk;
                    csk->iss = tcp_new_iss();
                    csk->snd_una = tsk->snd_una;
                    csk->rcv_nxt = tsk->rcv_nxt;
                    csk->snd_nxt = tsk->iss;
                    csk->rcv_wnd = tsk->rcv_wnd;
                    struct sock_addr *skaddr = (struct sock_addr *) malloc(sizeof(struct sock_addr));
                    skaddr->ip = htonl(cb->daddr);
                    skaddr->port = htons(cb->dport);
                    tcp_set_state(csk, TCP_SYN_RECV);
                    tcp_hash(csk);
                    tcp_send_control_packet(csk, TCP_SYN | TCP_ACK);
                    break;
                case TCP_SYN_RECV:
                    break;
                default:
                    printf("Recv SYN but current state is %d", tsk->state);
            }
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
                case TCP_ESTABLISHED:
                    if (tsk->cgt_state == OPEN) wake_up(tsk->wait_send);
                    break;
                case TCP_FIN_WAIT_1:
                    tcp_set_state(tsk, TCP_FIN_WAIT_2);
                    break;
                case TCP_LAST_ACK:
                    tcp_set_state(tsk, TCP_CLOSED);
                    if (!tsk->parent) tcp_bind_unhash(tsk);
                    tcp_unhash(tsk);
                    break;
                default:
                    printf("Unset state for ACK %d\n", tsk->state);
            }
            break;
        case TCP_PSH:
        case (TCP_PSH | TCP_ACK):
            if (tsk->state == TCP_SYN_RECV) tcp_set_state(tsk, TCP_ESTABLISHED);
            pthread_mutex_lock(&tsk->rcv_buf->lock);
            u32 seq_end = tsk->rcv_nxt;
            if (seq_end == cb->seq) {
                while (ring_buffer_free(tsk->rcv_buf) < cb->pl_len) {
                    wake_up(tsk->wait_read);
                    pthread_mutex_unlock(&tsk->rcv_buf->lock);
                    sleep_on(tsk->wait_recv);
                    pthread_mutex_lock(&tsk->rcv_buf->lock);
                }
                write_ring_buffer(tsk->rcv_buf, cb->payload, cb->pl_len);
                tsk->rcv_wnd -= cb->pl_len;
                seq_end = cb->seq_end;
                struct ofo_buffer *ofo, *q;
                list_for_each_entry_safe(ofo, q, &tsk->rcv_ofo_buf, list) {
                    if (seq_end < ofo->seq) break;
                    else {
                        struct ring_buffer *rbuf = ofo->tsk->rcv_buf;
                        int size = ofo->pl_len;
                        while (ring_buffer_free(rbuf) < size) {
                            wake_up(tsk->wait_read);
                            pthread_mutex_unlock(&tsk->rcv_buf->lock);
                            sleep_on(tsk->wait_recv);
                            pthread_mutex_lock(&tsk->rcv_buf->lock);
                        }
                        seq_end = ofo->seq_end;
                        write_ring_buffer(rbuf, ofo->payload, size);
                        tsk->rcv_wnd -= size;
                        list_delete_entry(&ofo->list);
                        free(ofo->payload);
                        free(ofo);
                    }
                }
                tsk->rcv_nxt = seq_end;
            } else if (seq_end < cb->seq) {
                write_ofo_buffer(tsk, cb);
            } else {
                pthread_mutex_unlock(&tsk->rcv_buf->lock);
                tcp_send_control_packet(tsk, TCP_ACK);
                break;
            }
            pthread_mutex_unlock(&tsk->rcv_buf->lock);
            if (tsk->wait_read->sleep) {
                wake_up(tsk->wait_read);
            }
            tcp_send_control_packet(tsk, TCP_ACK);
            if (tsk->cgt_state == OPEN && tsk->wait_send->sleep) {
                wake_up(tsk->wait_send);
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
                    tcp_set_timewait_timer(tsk);
                    break;
                case TCP_CLOSE_WAIT:
                    break;
                case TCP_LAST_ACK:
                    break;
                case TCP_FIN_WAIT_2:
                    tcp_set_state(tsk, TCP_TIME_WAIT);
                    tcp_send_control_packet(tsk, TCP_ACK);
                    tcp_set_timewait_timer(tsk);
                    break;
                default:
                    printf("Unset state for FIN %d\n", tsk->state);
            }
            break;
        default:
            printf("Unset flag %d\n", cb->flags);
    }
}
