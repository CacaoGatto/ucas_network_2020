#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#define SERVER_IP "192.192.255.63"
int main()
{
    int client_sockfd;
    int len;
    struct sockaddr_in address;//服务器端网络地址结构体 
    int result;
    char *ch = "abcdefg";
    client_sockfd = socket(AF_INET, SOCK_STREAM, 0);//建立客户端socket 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(SERVER_IP);
    address.sin_port = htons(8100);
    len = sizeof(address);
    result = connect(client_sockfd, (struct sockaddr *)&address, len);
    if(result == -1)
    {
        perror("oops: client2");
        exit(1);
    }
    //第一次读写
    write(client_sockfd, ch, 1);
    read(client_sockfd, ch, 1);
    printf("the first time: char from server = %s\n", ch);
    sleep(15);

    //第二次读写
    write(client_sockfd, ch, 1);
    read(client_sockfd, ch, 1);
    printf("the second time: char from server = %s\n", ch);

    close(client_sockfd);

    return 0;
}