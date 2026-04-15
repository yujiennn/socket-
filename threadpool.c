#include"threadpool.h"
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>

const int NUMBER=2;//每次增加或销毁的线程个数

typedef struct Task{
    void (*function)(void*);
    void* arg;
}Task;
struct ThreadPool{
    Task* taskQ;//任务队列
    int queueCapacity;//任务队列容量
    int queueSize;//任务队列当前大小
    int queueFront;//任务队列头索引
    int queueRear;//任务队列尾索引

    pthread_t managerID;//管理者线程ID
    pthread_t* threadIDs;//工作者线程ID数组
    int minNum;//最小线程数
    int maxNum;//最大线程数
    int busyNum;//忙线程数
    int liveNum;//存活线程数
    int exitNum;//要销毁的线程数
    pthread_mutex_t mutexPool;//线程池锁
    pthread_mutex_t mutexBusy;//忙线程锁
    int shutdown;//线程池是否关闭标志
    pthread_cond_t notFull;//任务队列不满条件变量
    pthread_cond_t notEmpty;//任务队列不空条件变量
};

ThreadPool* threadpoolCreate(int min, int max, int queueSize){
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    do{
        if(pool == NULL){
            printf("malloc threadpool failed\n");
            break;
        }
        pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t)*max);
        if(pool->threadIDs == NULL){
            printf("malloc threadIDs failed\n");
            break;
        }
        memset(pool->threadIDs,0,sizeof(pthread_t)*max);
        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min;
        pool->exitNum = 0;
        if(pthread_mutex_init(&(pool->mutexPool),NULL)!=0||
        pthread_mutex_init(&(pool->mutexBusy),NULL)!=0||
        pthread_cond_init(&(pool->notEmpty),NULL)!=0||
        pthread_cond_init(&(pool->notFull),NULL)!=0){
            printf("init mutex or cond failed\n");
            break;
        }
        pool->taskQ = (Task*)malloc(sizeof(Task)*queueSize);
        pool->queueCapacity = queueSize;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;
        pool->shutdown = 0;
        //创建线程
        pthread_create(&(pool->managerID),NULL,manager,(void*)pool);
        for(int i=0;i<min;++i){
            pthread_create(&(pool->threadIDs[i]),NULL,worker,(void*)pool);
        }  
        return pool;
    }while(0);
    //释放资源
    if(pool->threadIDs) free(pool->threadIDs);
    if(pool->taskQ) free(pool->taskQ);
    if(pool) free(pool);
       
    return NULL;
}
void* worker(void* arg){
    ThreadPool* pool = (ThreadPool*)arg;
    while(1){
        //加锁
        pthread_mutex_lock(&(pool->mutexPool));
        //任务队列为空，阻塞工作线程
        while(pool->queueSize == 0 && !pool->shutdown){
            pthread_cond_wait(&(pool->notEmpty),&(pool->mutexPool));
            if(pool->exitNum > 0)
            {
                pool->exitNum--;
                if(pool->liveNum > pool->minNum)
                {
                pool->liveNum--;
                }
                pthread_mutex_unlock(&(pool->mutexPool)); 
                threadExit(pool);
            }
            }
        //判断线程池是否被关闭
        if(pool->shutdown){
            pthread_mutex_unlock(&(pool->mutexPool));
            threadExit(pool);
        }
        //从任务队列取出任务
        Task task;
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;
        pthread_cond_signal(&(pool->notFull));//唤醒生产者
        pthread_mutex_unlock(&(pool->mutexPool));
        printf("thread %ld start working...\n",pthread_self());
        pthread_mutex_lock(&(pool->mutexBusy));
        pool->busyNum++;
        pthread_mutex_unlock(&(pool->mutexBusy));
        task.function(task.arg);
        free(task.arg);
        task.arg = NULL;
        printf("thread %ld end working...\n",pthread_self());
        pthread_mutex_lock(&(pool->mutexBusy));
        pool->busyNum--;
        pthread_mutex_unlock(&(pool->mutexBusy));
    }
    return NULL;
}




void* manager(void* arg){   
    ThreadPool* pool = (ThreadPool*)arg;
    while(!pool->shutdown)
    {
        sleep(3);
        pthread_mutex_lock(&(pool->mutexPool));
        int queueSize = pool->queueSize;
        int liveNum = pool->liveNum;

        pthread_mutex_unlock(&(pool->mutexPool));
        pthread_mutex_lock(&(pool->mutexBusy));
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&(pool->mutexBusy));

        //添加线程
        //任务数大于最小线程数且存活线程数小于最大线程数
        if(queueSize>liveNum&&liveNum<pool->maxNum)
        {
            pthread_mutex_lock(&(pool->mutexPool));
            int counter=0;
            for(int i=0;i<pool->maxNum&&counter<NUMBER&&pool->liveNum<pool->maxNum;++i)
            {
                if(pool->threadIDs[i]==0)
                {
                pthread_create(&(pool->threadIDs[i]),NULL,worker,pool);
                counter++;
                pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&(pool->mutexPool));
        }
        //销毁线程
        //忙线程数*2小于存活线程数且存活线程数大
        if(busyNum*2<liveNum&&liveNum>pool->minNum)
        {
            pthread_mutex_lock(&(pool->mutexPool));
            pool->exitNum=NUMBER;
            pthread_mutex_unlock(&(pool->mutexPool));
            for(int i=0;i<NUMBER;++i)
            {
                pthread_cond_signal(&(pool->notEmpty));
            }
        }
    }
    return NULL;
}
void threadExit(ThreadPool* pool){
    pthread_t tid = pthread_self();
    for(int i=0;i<pool->maxNum;++i){
        if(pool->threadIDs[i]==tid){
            pool->threadIDs[i]=0;
            printf("thread %ld exit\n",tid);
            break;
        }
    }
    pthread_exit(NULL);
}
void threadpoolAdd(ThreadPool* pool,void (*function)(void*),void* arg){
    pthread_mutex_lock(&(pool->mutexPool));
    //任务队列满，等待
    while(pool->queueSize == pool->queueCapacity && !pool->shutdown){
        pthread_cond_wait(&(pool->notFull),&(pool->mutexPool));
    }
    //判断线程池是否被关闭
    if(pool->shutdown){
        pthread_mutex_unlock(&(pool->mutexPool));
        return;
    }
    //添加任务到任务队列
    pool->taskQ[pool->queueRear].function = function;
    pool->taskQ[pool->queueRear].arg = arg;
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;
    //通知工作线程有任务可做
    pthread_cond_signal(&(pool->notEmpty));
    pthread_mutex_unlock(&(pool->mutexPool));
}
int threadpoolBusyNum(ThreadPool* pool){
    int busyNum = 0;
    pthread_mutex_lock(&(pool->mutexBusy));
    busyNum = pool->busyNum;
    pthread_mutex_unlock(&(pool->mutexBusy));
    return busyNum;
}
int threadpoolAliveNum(ThreadPool* pool)
{
    pthread_mutex_lock(&(pool->mutexPool));
    int liveNum = pool->liveNum;
    pthread_mutex_unlock(&(pool->mutexPool));
    return liveNum;
}
int threadpoolDestroy(ThreadPool* pool){
    if(pool == NULL){
        return -1;
    }
    pool->shutdown = 1;
    //销毁管理者线程
    pthread_join(pool->managerID,NULL);
    //唤醒所有工作线程
    for(int i=0;i<pool->liveNum;++i){
        pthread_cond_broadcast(&(pool->notEmpty));
    }
    //销毁工作线程
    for(int i=0;i<pool->maxNum;++i){
        if(pool->threadIDs[i]!=0){
            pthread_join(pool->threadIDs[i],NULL);
        }
    }
    if(pool->taskQ){
        free(pool->taskQ);
    }
    if(pool->threadIDs){
        free(pool->threadIDs);
    }
    pthread_mutex_destroy(&(pool->mutexPool));
    pthread_mutex_destroy(&(pool->mutexBusy));
    pthread_cond_destroy(&(pool->notEmpty));
    pthread_cond_destroy(&(pool->notFull));
    free(pool);
    pool = NULL;
    return 0;
}