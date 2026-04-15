#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<pthread.h>
#include"threadpool.h"
struct SockInfo
{
    struct sockaddr_in addr;//ip和端口号
    int fd;//文件描述符
};
typedef struct PoolInfo
{
    ThreadPool* pool;
    int fd;
}PoolInfo; 
struct SockInfo infos[512];
void working(void* arg);
void acceptConn(void* arg);
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
    
//     struct sockaddr_in
//   {
//     __SOCKADDR_COMMON (sin_);
//     in_port_t sin_port;			/* 端口.  */
//     struct in_addr sin_addr;		/* ip地址  */

//     /* Pad to size of `struct sockaddr'.  填充8字节*/
//     unsigned char sin_zero[sizeof (struct sockaddr)
// 			   - __SOCKADDR_COMMON_SIZE
// 			   - sizeof (in_port_t)
// 			   - sizeof (struct in_addr)];
//   };
    
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = INADDR_ANY;//本机任意ip地址0.0.0.0
    //sockaddr_in结构体强制转换成sockaddr结构体，传入bind函数中
    int ret=bind(fd,(struct sockaddr*)&saddr,sizeof(saddr));//绑定地址必须大端的，网络字节序
    //bind成功返回0，失败返回-1
    if(ret == -1)
    {
        perror("bind");
        return -1;
    }
    ret=listen(fd,128);
    if(ret == -1)
    {
        perror("listen");
        return -1;
    }
    //创建线程池
    ThreadPool* pool=threadpoolCreate(3,8,100);
    PoolInfo* info=(PoolInfo*)malloc(sizeof(PoolInfo));
    info->fd=fd;
    info->pool=pool;
    threadpoolAdd(pool,acceptConn,(void*)info);
    pthread_exit(NULL);
    return 0;
}
void acceptConn(void* arg){
    //3.等待客户端连接
    PoolInfo* poolInfo=(PoolInfo*)arg;
    int addrlen=sizeof(struct sockaddr_in);
    while(1)
    {   struct SockInfo* pinfo=NULL;
        pinfo=(struct SockInfo*)malloc(sizeof(struct SockInfo));
        pinfo->fd=accept(poolInfo->fd,(struct sockaddr*)&pinfo->addr,&addrlen);
        //accept成功返回用于通信的文件描述符，失败返回-1
        if(pinfo->fd == -1)
        {
            perror("accept");
            break;
        }
        //添加通信任务
        threadpoolAdd(poolInfo->pool,working,(void*)pinfo);
       
    }
    close(poolInfo->fd);
}
void working(void* arg)
{
    struct SockInfo* pinfo = (struct SockInfo*)arg;
  //连接建立成功，打印客户端的ip和端口号
    char ip[32];
    printf("client ip:%s,port:%d\n",inet_ntop(AF_INET,&pinfo->addr.sin_addr,ip,sizeof(ip)),ntohs(pinfo->addr.sin_port));
    //4.通信
    while(1)
    {
       char buf[1024]={0};
       int len=recv(pinfo->fd,buf,sizeof(buf),0);//0表示阻塞接收，返回值为实际接收的字节数
         if(len >0)
         {  
            printf("client say:%s\n",buf);
            send(pinfo->fd,buf,len,0);
        }
        else if(len == 0)
        {
            printf("client disconnect...\n");
            break;
        }
        else
        {
            perror("recv");
            break;
        }
    }
    close(pinfo->fd);
}