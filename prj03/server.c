#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define PORT_NUM 80
#define MAXC 8

int main(int argc, const char *argv[])
{
    int s, cs, fd[MAXC];
    struct sockaddr_in server, client;
    char msg[2000];
    memset(fd, 0, sizeof(fd));

    // create socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Create socket failed");
		return -1;
    }
    printf("Socket created\n");

    int yes = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        perror("Setsockopt error");
        return -1;
    }

    // prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT_NUM);

    // bind
    if (bind(s,(struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        return -1;
    }
    printf("Bind done\n");

    // listen
    listen(s, MAXC);
    printf("Waiting for incoming connections...\n");

    fd_set fds;
    int connection = 0;
    struct timeval timeout;
    int maxs = s;

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(s, &fds);

        timeout.tv_sec = 30;
        timeout.tv_usec = 0;

        for (int i = 0; i < MAXC; i++) {
            if (fd[i] != 0) FD_SET(fd[i], &fds);
        }

        int ready = select(maxs + 1, &fds, NULL, NULL, &timeout);
        if (ready < 0) {
            perror("Select error");
            break;
        }
        else if (ready == 0) {
            printf("Timeout!\n");
            continue;
        }
        printf("get!!!\n");
        for (int i = 0; i < MAXC; i++) {
            if (FD_ISSET(fd[i], &fds)) {
                int msg_len = 0;
                // receive a message from client
                if ((msg_len = recv(fd[i], msg, sizeof(msg), 0)) > 0) {
                    // send the message back to client
                    send(fd[i], msg, msg_len, 0);
                    printf("Echo client [%d].\n", i);
                }
                else {
                    if (msg_len == 0) {
                        printf("Client disconnected\n");
                    }
                    else { // msg_len < 0
                        perror("Recv failed");
                    }
                    close(fd[i]);
                    FD_CLR(fd[i], &fds);
                    fd[i] = 0;
                    connection--;
                }
            }
        }
        if (FD_ISSET(s, &fds)) {
            int c = sizeof(struct sockaddr_in);
            if ((cs = accept(s, (struct sockaddr *)&client, (socklen_t *)&c)) <= 0) {
                perror("Accept failed");
                continue;
            }

            if (connection < MAXC) {
                int i = 0;
                for  (i = 0; i < MAXC; i++) {
                    if (fd[i] == 0) {
                        fd[i] = cs;
                        break;
                    }
                }
                connection++;
                printf("Client [%d]: connection accepted\n", i);
                if (cs > maxs) maxs = cs;
            }
            else {
                printf("Connection is full!\n");
                send(cs, "Connection is full!", 20, 0);
                close(cs);
                continue;
            }
        }
    }
/*
    // accept connection from an incoming client
    int c = sizeof(struct sockaddr_in);
    if ((fd = accept(s, (struct sockaddr *)&client, (socklen_t *)&c)) < 0) {
        perror("accept failed\n");
        return -1;
    }
    printf("connection accepted\n");

	int msg_len = 0;
    // receive a message from client
    while ((msg_len = recv(fd, msg, sizeof(msg), 0)) > 0) {
        // send the message back to client
        write(fd, msg, msg_len);
    }

    if (msg_len == 0) {
        printf("client disconnected\n");
    }
    else { // msg_len < 0
        perror("recv failed\n");
		return -1;
    }
*/
    return 0;
}
