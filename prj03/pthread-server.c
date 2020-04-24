#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT_NUM 80
#define MAXC 8
#define BUFSIZE 3000
#define CONTENT_SIZE 2000

char ok[] = "HTTP/1.1 200 OK\r\nContent-Length: ";
char fail[] = "HTTP/1.1 404 File Not Found\r\n\r\n";

void *routine(void* cs) {
    char buf[BUFSIZE], filename[240], readbuf[CONTENT_SIZE];
    int len = 0;
    int fd = *((int*)cs);

    while (1) {
        // receive a message from client
        if ((len = recv(fd, buf, sizeof(buf), 0)) > 0) {
            sscanf(buf, "%*s /%[^ ]", filename);
            // send the file back to client
            FILE *fp = fopen(filename, "r");
            if (fp) {
                printf("Begin to transfer %s to the client.\n", filename);
                unsigned long rret = 0;
                memset(buf, 0, sizeof(buf));
                memset(readbuf, 0, sizeof(readbuf));
                while ((rret = fread(readbuf, sizeof(char), CONTENT_SIZE, fp)) > 0) {
                    sprintf(buf, "%s%lu\r\n\r\n%s", ok, rret, readbuf);
                    if (send(fd, buf, strlen(buf), 0) < 0) {
                        printf("Send failed.\n");
                        break;
                    }
                    memset(buf, 0, sizeof(buf));
                }
                fclose(fp);
                printf("Transfer finished.\n");
            } else {
                printf("File %s not found.\n", filename);
                strcpy(buf, fail);
                if (send(fd, buf, strlen(buf), 0) < 0) {
                    printf("Send failed.\n");
                }
                memset(buf, 0, sizeof(buf));
            }
        } else {
            if (len == 0) {
                printf("Client disconnected\n");
            } else { // len < 0
                perror("Recv failed");
            }
            close(fd);
            free(cs);
            pthread_exit(NULL);
        }
    }
}

int main(int argc, const char *argv[])
{
    int s;
    int* cs;
    struct sockaddr_in server, client;

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

    while (1) {
        pthread_t thread;
        cs = (int*)malloc(sizeof(int));

        // accept connection from an incoming client
        int c = sizeof(struct sockaddr_in);
        if ((*cs = accept(s, (struct sockaddr *)&client, (socklen_t *)&c)) < 0) {
            perror("Accept failed");
            return -1;
        }
        printf("Connection from %s accepted.\n", inet_ntoa(client.sin_addr));
        if (pthread_create(&thread, NULL, routine, cs) != 0) {
            perror("Pthread create failed");
            return -1;
        }
    }

    return 0;
}
