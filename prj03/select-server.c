#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define PORT_NUM 80
#define MAXC 8
#define BUFSIZE 3000
#define CONTENT_SIZE 2000

char ok[] = "HTTP/1.1 200 OK\r\nContent-Length: ";
char fail[] = "HTTP/1.1 404 File Not Found\r\n\r\n";

int main(int argc, const char *argv[])
{
    int s, cs, fd[MAXC];
    struct sockaddr_in server, client;
    char buf[BUFSIZE], readbuf[CONTENT_SIZE];
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
    char filename[240];

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
        //printf("get!!!\n");
        for (int i = 0; i < MAXC; i++) {
            if (FD_ISSET(fd[i], &fds)) {
                int len = 0;
                // receive a filename from client
                if ((len = recv(fd[i], buf, sizeof(buf), 0)) > 0) {
                    sscanf(buf, "%*s /%[^ ]", filename);
                    // send the file back to client
                    FILE *fp = fopen(filename, "r");
                    if (fp) {
                        printf("Begin to transfer %s to client [%d].\n", filename, i + 1);
                        unsigned long rret = 0;
                        memset(buf, 0, sizeof(buf));
                        memset(readbuf, 0, sizeof(readbuf));
                        while ((rret = fread(readbuf, sizeof(char), CONTENT_SIZE, fp)) > 0) {
                            sprintf(buf, "%s%lu\r\n\r\n%s", ok, rret, readbuf);
                            if (send(fd[i], buf, strlen(buf), 0) < 0) {
                                printf("Send failed.\n");
                                break;
                            }
                            memset(buf, 0, sizeof(buf));
                        }
                        fclose(fp);
                        printf("Transfer finished.\n");
                    }
                    else {
                        printf("File %s not found.\n", filename);
                        strcpy(buf, fail);
                        if (send(fd[i], buf, strlen(buf), 0) < 0) {
                            printf("Send failed.\n");
                        }
                        memset(buf, 0, sizeof(buf));
                    }
                }
                else {
                    if (len == 0) {
                        printf("Client [%d] disconnected\n", i + 1);
                    }
                    else { // len < 0
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
                printf("Client [%d]: connection accepted\n", i + 1);
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
    return 0;
}
