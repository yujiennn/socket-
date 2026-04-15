#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

typedef struct ThreadPool ThreadPool;
//创建线程池并初始化
ThreadPool* threadpoolCreate(int min, int max, int queueSize);
//销毁线程池
int threadpoolDestroy(ThreadPool* pool);
//给线程池添加任务
void threadpoolAdd(ThreadPool* pool,void (*function)(void*),void* arg);
//获取线程池中工作的线程个数
int threadpoolBusyNum(ThreadPool* pool);

//获取线程池中活着的线程的个数
int threadpoolAliveNum(ThreadPool* pool);

void* worker(void* arg);
void* manager(void* arg);
void threadExit(ThreadPool* pool);
#endif /* _THREADPOOL_H_ */