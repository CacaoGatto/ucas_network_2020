#include "tcp.h"
#include "tcp_timer.h"
#include "tcp_sock.h"

#include <stdio.h>
#include <unistd.h>

static struct list_head timer_list;
pthread_mutex_t timer_lock;

// scan the timer_list, find the tcp sock which stays for at 2*MSL, release it
void tcp_scan_timer_list() {
    // TODO: implement %s please.\n, __FUNCTION__
    struct tcp_timer *pos, *q;
    pthread_mutex_lock(&timer_lock);
    list_for_each_entry_safe(pos, q, &timer_list, list) {
        if (pos->type == 0) {
            pos->timeout -= TCP_TIMER_SCAN_INTERVAL;
            if (pos->timeout <= 0) {
                list_delete_entry(&pos->list);
                struct tcp_sock *tsk = timewait_to_tcp_sock(pos);
                tcp_set_state(tsk, TCP_CLOSED);
                if (!tsk->parent) tcp_bind_unhash(tsk);
                tcp_unhash(tsk);
            }
        } else {
            struct tcp_sock *tsk = retranstimer_to_tcp_sock(pos);
            struct send_buffer *buf;
            pthread_mutex_lock(&tsk->send_lock);
            list_for_each_entry(buf, &tsk->send_buf, list) {
                buf->timeout -= TCP_TIMER_SCAN_INTERVAL;
                if (!buf->timeout) {
                    if (tsk->cgt_state != LOSS) tsk->recovery_point = tsk->snd_nxt;
                    tsk->cgt_state = LOSS;
                    tsk->ssthresh = (tsk->cwnd + 1) / 2;
                    tsk->cwnd = 1;
                    tsk->snd_wnd = MSS;
                    update_log(tsk);
                    if (buf->times++ == 3) {
                        printf("Packet Loss!!!\n");
                        buf->times = 1;
                        buf->timeout = TCP_RETRANS_INTERVAL_INITIAL;
                    } else {
                        char *temp = (char *) malloc(buf->len * sizeof(char));
                        memcpy(temp, buf->packet, buf->len);
                        ip_send_packet(temp, buf->len);
                        if (buf->times == 2) buf->timeout = 2 * TCP_RETRANS_INTERVAL_INITIAL;
                        else buf->timeout = 4 * TCP_RETRANS_INTERVAL_INITIAL;
                    }
                }
            }
            pthread_mutex_unlock(&tsk->send_lock);
        }
    }
    pthread_mutex_unlock(&timer_lock);
}

// set the timewait timer of a tcp sock, by adding the timer into timer_list
void tcp_set_timewait_timer(struct tcp_sock *tsk) {
    // TODO: implement %s please.\n, __FUNCTION__
    tsk->timewait.type = 0;
    tsk->timewait.timeout = TCP_TIMEWAIT_TIMEOUT;
    pthread_mutex_lock(&timer_lock);
    list_add_tail(&tsk->timewait.list, &timer_list);
    pthread_mutex_unlock(&timer_lock);
    tsk->ref_cnt++;
}

void tcp_set_retrans_timer(struct tcp_sock *tsk) {
    tsk->retrans_timer.type = 1;
    tsk->retrans_timer.timeout = TCP_RETRANS_INTERVAL_INITIAL;
    pthread_mutex_lock(&timer_lock);
    struct tcp_timer *pos;
    list_for_each_entry(pos, &timer_list, list) {
        if (retranstimer_to_tcp_sock(pos) == tsk) {
            pthread_mutex_unlock(&timer_lock);
            return;
        }
    }
    list_add_tail(&tsk->retrans_timer.list, &timer_list);
    pthread_mutex_unlock(&timer_lock);
    tsk->ref_cnt++;
}

void tcp_unset_retrans_timer(struct tcp_sock *tsk) {
    pthread_mutex_lock(&timer_lock);
    list_delete_entry(&tsk->retrans_timer.list);
    pthread_mutex_unlock(&timer_lock);
    tsk->ref_cnt--;
}

// scan the timer_list periodically by calling tcp_scan_timer_list
void *tcp_timer_thread(void *arg) {
    init_list_head(&timer_list);
    pthread_mutex_init(&timer_lock, NULL);
    timer_thread_init = 1;
    while (1) {
        usleep(TCP_TIMER_SCAN_INTERVAL);
        tcp_scan_timer_list();
    }

    return NULL;
}
