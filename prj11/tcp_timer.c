#include "tcp.h"
#include "tcp_timer.h"
#include "tcp_sock.h"

#include <stdio.h>
#include <unistd.h>

static struct list_head timer_list;

// scan the timer_list, find the tcp sock which stays for at 2*MSL, release it
void tcp_scan_timer_list()
{
	// TODO: implement %s please.\n, __FUNCTION__
	struct tcp_timer *pos, *q;
	list_for_each_entry_safe(pos, q, &timer_list, list) {
	    pos->timeout -= TCP_TIMER_SCAN_INTERVAL;
	    if (pos->timeout <= 0) {
	        list_delete_entry(&pos->list);
	        struct tcp_sock *tsk = timewait_to_tcp_sock(pos);
            tcp_set_state(tsk, TCP_CLOSED);
	        if (!tsk->parent) tcp_bind_unhash(tsk);
            tcp_unhash(tsk);
	    }
	}
}

// set the timewait timer of a tcp sock, by adding the timer into timer_list
void tcp_set_timewait_timer(struct tcp_sock *tsk)
{
	// TODO: implement %s please.\n, __FUNCTION__
	tsk->timewait.type = 0;
	tsk->timewait.timeout = TCP_TIMEWAIT_TIMEOUT;
	list_add_tail(&tsk->timewait.list, &timer_list);
	tsk->ref_cnt++;
}

// scan the timer_list periodically by calling tcp_scan_timer_list
void *tcp_timer_thread(void *arg)
{
	init_list_head(&timer_list);
	while (1) {
		usleep(TCP_TIMER_SCAN_INTERVAL);
		tcp_scan_timer_list();
	}

	return NULL;
}
