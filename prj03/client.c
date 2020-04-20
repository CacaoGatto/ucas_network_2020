/* client application */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT_NUM 80
 
int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char filename[1000], buf[2000];
     
    // create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Create socket failed");
		return -1;
    }
    printf("Socket created\n");
     
    server.sin_addr.s_addr = inet_addr("10.0.0.1");
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
         
        // send some data
        if (send(sock, filename, strlen(filename), 0) < 0) {
            printf("Send failed");
            return 1;
        }

        FILE *fp = fopen("test.txt", "w");

        // receive a reply from the server
		int len = 0;
		memset(buf, 0, sizeof(buf));
        while ((len = recv(sock, buf, 2000, 0))) {
            if (len < 0) {
                printf("Recv failed!\n");
                break;
            }
            printf("get!\n");
            unsigned long wret = fwrite(buf, sizeof(char), len, fp);
            if (wret < len) {
                printf("File writing fail!\n");
                break;
            }
            memset(buf, 0, sizeof(buf));
        }

        printf("File received!\n");
        fclose(fp);
        break;
    }
     
    close(sock);
    printf("Connect closed\n");
    return 0;
}
