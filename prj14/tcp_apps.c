#include "tcp_sock.h"

#include "log.h"

#include <stdlib.h>
#include <unistd.h>

#define CONTENT_SIZE 500

#define FILE_TRANSFER
#define NO_ECHO

#ifdef FILE_TRANSFER
#define INPUT_NAME "client-input.dat"
#define OUTPUT_NAME "server-output.dat"
#endif


// tcp server application, listens to port (specified by arg) and serves only one
// connection request
void *tcp_server(void *arg)
{
	u16 port = *(u16 *)arg;
	struct tcp_sock *tsk = alloc_tcp_sock();

	struct sock_addr addr;
	addr.ip = htonl(0);
	addr.port = port;
	if (tcp_sock_bind(tsk, &addr) < 0) {
		log(ERROR, "tcp_sock bind to port %hu failed", ntohs(port));
		exit(1);
	}

	if (tcp_sock_listen(tsk, 3) < 0) {
		log(ERROR, "tcp_sock listen failed");
		exit(1);
	}

	log(DEBUG, "listen to port %hu.", ntohs(port));

	struct tcp_sock *csk = tcp_sock_accept(tsk);

	log(DEBUG, "accept a connection.");

	char rbuf[CONTENT_SIZE + 1];
    int rlen = 0;

#ifdef FILE_TRANSFER
    printf("Begin to receive %s from client.\n", OUTPUT_NAME);
    FILE *fp = fopen(OUTPUT_NAME, "w+");
    unsigned long wlen = 0;
    while (1) {
        memset(rbuf, 0, sizeof(rbuf));
        rlen = tcp_sock_read(csk, rbuf, CONTENT_SIZE);
        if (rlen == 0) {
            log(DEBUG, "tcp_sock_read return 0, finish transmission.");
            break;
        }
        else if (rlen > 0) {
#ifndef NO_ECHO
            rbuf[rlen] = '\0';
            printf("[Recv] %s\n", rbuf);
#endif
            wlen = fwrite(rbuf, sizeof(char), rlen, fp);
            fflush(fp);
            if (wlen < rlen) {
                printf("File writing fail!\n");
                break;
            }
        }
        else {
            log(DEBUG, "tcp_sock_read return negative value, something goes wrong.");
            exit(1);
        }
    }
    // printf("File received! Final rlen: %d ; Final wlen: %lu\n", rlen, wlen);
    sleep(1);
    fclose(fp);
#else
	char wbuf[1024];

	while (1) {
		rlen = tcp_sock_read(csk, rbuf, CONTENT_SIZE);
		if (rlen == 0) {
			log(DEBUG, "tcp_sock_read return 0, finish transmission.");
			break;
		} 
		else if (rlen > 0) {
			rbuf[rlen] = '\0';
			sprintf(wbuf, "server echoes: %s", rbuf);
			if (tcp_sock_write(csk, wbuf, strlen(wbuf)) < 0) {
				log(DEBUG, "tcp_sock_write return negative value, something goes wrong.");
				exit(1);
			}
		}
		else {
			log(DEBUG, "tcp_sock_read return negative value, something goes wrong.");
			exit(1);
		}
	}
#endif

	log(DEBUG, "close this connection.");

	tcp_sock_close(csk);
	
	return NULL;
}

// tcp client application, connects to server (ip:port specified by arg), each
// time sends one bulk of data and receives one bulk of data 
void *tcp_client(void *arg)
{
	struct sock_addr *skaddr = arg;

	struct tcp_sock *tsk = alloc_tcp_sock();

	if (tcp_sock_connect(tsk, skaddr) < 0) {
		log(ERROR, "tcp_sock connect to server ("IP_FMT":%hu)failed.", \
				NET_IP_FMT_STR(skaddr->ip), ntohs(skaddr->port));
		exit(1);
	}

    char rbuf[CONTENT_SIZE + 1];
    int rlen = 0;

#ifdef FILE_TRANSFER
    FILE *fp = fopen(INPUT_NAME, "r");
    if (fp) {
        printf("Begin to transfer %s to server.\n", INPUT_NAME);
        char wbuf[CONTENT_SIZE];
        int wlen;
        memset(wbuf, 0, sizeof(wbuf));
        while ((wlen = fread(wbuf, sizeof(char), CONTENT_SIZE, fp)) > 0) {
#ifndef NO_ECHO
            printf("[Send] %s\n", wbuf);
#endif
            if (tcp_sock_write(tsk, wbuf, wlen) < 0) {
                printf("Unexpected ERROR!!!!!!\n");
                break;
            }
            // usleep(1000);
            memset(wbuf, 0, sizeof(wbuf));
            if (feof(fp)) break;
        }
        fclose(fp);
        printf("Transfer finished.\n");
    }
    else {
        printf("File %s not found.\n", INPUT_NAME);
    }
    sleep(1);
#else
    char *wbuf = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int wlen = strlen(wbuf);

	int n = 10;
	for (int i = 0; i < n; i++) {
		if (tcp_sock_write(tsk, wbuf + i, wlen - n) < 0)
			break;

		rlen = tcp_sock_read(tsk, rbuf, CONTENT_SIZE);
		if (rlen == 0) {
			log(DEBUG, "tcp_sock_read return 0, finish transmission.");
			break;
		}
		else if (rlen > 0) {
			rbuf[rlen] = '\0';
			fprintf(stdout, "%s\n", rbuf);
		}
		else {
			log(DEBUG, "tcp_sock_read return negative value, something goes wrong.");
			exit(1);
		}
		sleep(1);
	}
#endif

	tcp_sock_close(tsk);

	return NULL;
}
