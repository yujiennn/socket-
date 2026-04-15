#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
int main()
{
    //1.创建套接字
    int fd = socket(AF_INET,SOCK_STREAM,0);//tcp
    if(fd == -1)
    {
        perror("socket");
        return -1;
    }
    //2.绑定地址信息
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    inet_pton(AF_INET, "192.168.88.131", &saddr.sin_addr);//指定服务器的ip地址
    int ret=connect(fd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(ret == -1)
    {
        perror("connect");
        return -1;
    }
    //4.通信
    int number=0;
    while(1)
    {//发送数据
       char buf[1024]={0};
       sprintf(buf,"hello server,%d...\n",number++);
       send(fd,buf,strlen(buf)+1,0);

       //接收数据
       int len=recv(fd,buf,sizeof(buf),0);
         if(len >0)
         {  
            printf("server say:%s\n",buf);
            send(fd,buf,len,0);
        }
        else if(len == 0)
        {
            printf("server disconnect...\n");
            break;
        }
        else
        {
            perror("recv");
            break;
        }
        sleep(1);
    }
    close(fd);
    return 0;
}