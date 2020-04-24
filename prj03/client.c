/* client application */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT_NUM 80
#define BUFSIZE 3000

char request_head1[] = "GET ";
char request_head2[] = " HTTP/1.1\r\nHost: ";

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char filename[240], buf[BUFSIZE], url[500];

    memset(url, 0, sizeof(url));
    printf("URL to connect : ");
    scanf("%s", url);

    // create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Create socket failed");
		return -1;
    }
    printf("Socket created\n");

    server.sin_addr.s_addr = inet_addr(url);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_NUM);

    // connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        return 1;
    }

    printf("Connected\n");

    while(1) {

        memset(filename, 0, sizeof(filename));
        printf("Filename to download : ");
        scanf("%s", filename);

        // prepare http packet
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s/%s%s%s\r\n\r\n", request_head1, filename, request_head2, url);

        // send some data
        if (send(sock, buf, strlen(buf), 0) < 0) {
            printf("Send failed\n");
            return 1;
        }

        sprintf(buf, "download/%s", filename);
        strcpy(filename, buf);

        // receive a reply from the server
		int len = 0;
		memset(buf, 0, sizeof(buf));
        while ((len = recv(sock, buf, BUFSIZE, 0)) != 0) {
            if (len < 0) {
                printf("Recv failed!\n");
                break;
            }
            printf("%s\n", buf);
            char state[4];
            sscanf(buf, "%*[^ ] %[^ ]", state);
            if (strcmp(state, "404") != 0) {
                printf("%s OK!\n", state);
                int start = 0;
                while (start < len) {
                    if (buf[start] == '\r' && buf[start + 2] == '\r' && buf[start + 1] == '\n' && buf[start + 3] == '\n') {
                        start += 4;
                        break;
                    }
                    start++;
                }
                FILE *fp = fopen(filename, "a+");
                unsigned long wret = fwrite(&buf[start], sizeof(char), len - start, fp);
                if (wret < len - start) {
                    printf("File writing fail!\n");
                    break;
                }
                printf("File received!\n");
                fclose(fp);
                memset(buf, 0, sizeof(buf));
                break;
            }
            else {
                printf("404 File Not Found.\n");
                break;
            }
        }
        // break
        if (len == 0) break;
    }

    close(sock);
    printf("Connect closed\n");
    return 0;
}
