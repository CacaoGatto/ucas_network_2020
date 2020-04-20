/*使用select函数可以以非阻塞的方式和多个socket通信。程序只是演示select函数的使用，连接数达到最大值后会终止程序。
1. 程序使用了一个数组fd，通信开始后把需要通信的多个socket描述符都放入此数组
2. 首先生成一个叫sock_fd的socket描述符，用于监听端口。
3. 将sock_fd和数组fd中不为0的描述符放入select将检查的集合fdsr。
4. 处理fdsr中可以接收数据的连接。如果是sock_fd，表明有新连接加入，将新加入连接的socket描述符放置到fd。 */ 
// select_server.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MYPORT 8100 //连接时使用的端口

#define MAXCLINE 5 //连接队列中的个数

#define BUF_SIZE 200

int fd[MAXCLINE]; //连接的fd

int conn_amount; //当前的连接数
​
void showclient()
{
    int i;
    printf("client amount:%d\n",conn_amount);
    for(i=0;i<MAXCLINE;i++)
    {
        printf("[%d]:%d ",i,fd[i]);
    }
    printf("\n\n");
}
int main(void)
{
    int sock_fd,new_fd; //监听套接字 连接套接字
    struct sockaddr_in server_addr; // 服务器的地址信息
    struct sockaddr_in client_addr; //客户端的地址信息
    socklen_t sin_size;
    int yes = 1;
    char buf[BUF_SIZE];
    int ret;
    int i;
  //建立sock_fd套接字
    if((sock_fd = socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        perror("setsockopt");
        exit(1);
    }
    printf("sockect_fd = %d\n", sock_fd);
    //设置套接口的选项 SO_REUSEADDR 允许在同一个端口启动服务器的多个实例
    // setsockopt的第二个参数SOL SOCKET 指定系统中，解释选项的级别 普通套接字
    if(setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1)
    {
        perror("setsockopt error \n");
        exit(1);
    }

    server_addr.sin_family = AF_INET; //主机字节序
    server_addr.sin_port = htons(MYPORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;//通配IP
    memset(server_addr.sin_zero,'\0',sizeof(server_addr.sin_zero));
    if(bind(sock_fd,(struct sockaddr *)&server_addr,sizeof(server_addr)) == -1)
    {
        perror("bind error!\n");
        exit(1);
    }
    if(listen(sock_fd,MAXCLINE)==-1)
    {
        perror("listen error!\n");
        exit(1);
    }
    printf("listen port %d\n",MYPORT);
    fd_set fdsr; //文件描述符集的定义
    int maxsock;
    struct timeval tv;
    conn_amount =0;
    sin_size = sizeof(client_addr);
    maxsock = sock_fd;
    while(1)
    {
        //这两部是非常重要的，不可缺少的，缺少了有可能导致状态不会更新
        FD_ZERO(&fdsr);
        FD_SET(sock_fd,&fdsr);

        //超时的设定，这里也可以不需要设置时间，将这个参数设置为NULL,表明此时select为阻塞模式
        tv.tv_sec = 30;
        tv.tv_usec =0;

        //将所有的连接全部加到这个这个集合中，可以监测客户端是否有数据到来
        for(i = 0; i < MAXCLINE; i++)
        {
            if(fd[i]!=0)
            {
                FD_SET(fd[i],&fdsr);
            }
        }
        //如果文件描述符中有连接请求 会做相应的处理，实现I/O的复用 多用户的连接通讯
        ret = select(maxsock +1,&fdsr,NULL,NULL,&tv);
        if(ret <0) //没有找到有效的连接 失败
        {
            perror("select error!\n");
            break;
        }
        else if(ret ==0)// 指定的时间到，
        {
            printf("timeout \n");
            continue;
        }
        //下面这个循环是非常必要的，因为你并不知道是哪个连接发过来的数据，所以只有一个一个去找。
        for(i=0;i<conn_amount;i++)
        {
            if(FD_ISSET(fd[i],&fdsr))
            {
                ret = recv(fd[i],buf,sizeof(buf),0);
                //如果客户端主动断开连接，会进行四次挥手，会出发一个信号，此时相应的套接字会有数据返回，告诉select，我的客户断开了，你返回-1

                if(ret <=0) //客户端连接关闭，清除文件描述符集中的相应的位
                {
                    printf("client[%d] close\n",i);
                    close(fd[i]);
                    FD_CLR(fd[i],&fdsr);
                    fd[i]=0;
                    conn_amount--;

                }
                //否则有相应的数据发送过来 ，进行相应的处理
                else
                {
                    if(ret <BUF_SIZE)
                        memset(&buf[ret],'\0',1);
                    printf("client[%d] send:%s\n",i,buf);
                }
            }
        }
        if(FD_ISSET(sock_fd,&fdsr))
        {
            new_fd = accept(sock_fd,(struct sockaddr *)&client_addr,&sin_size);
            if(new_fd <=0)
            {
                perror("accept error\n");
                continue;
            }

            //添加新的fd 到数组中 判断有效的连接数是否小于最大的连接数，如果小于的话，就把新的连接套接字加入集合
            if(conn_amount < MAXCLINE)
            {
                for(i = 0; i < MAXCLINE; i++)
                {
                    if(fd[i]==0)
                    {
                        fd[i] = new_fd;
                        break;
                    }
                }
                conn_amount++;
                printf("new connection client[%d]%s:%d\n",conn_amount,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
                if(new_fd > maxsock)
                {
                    maxsock = new_fd;
                }
            }

            else
            {
                printf("max connections arrive ,exit\n");
                send(new_fd,"bye",4,0);
                close(new_fd);
                continue;
            }
        }
        showclient();
    }

    for(i=0;i<MAXCLINE;i++)
    {
        if(fd[i]!=0)
        {
            close(fd[i]);
        }
    }
    exit(0);
}