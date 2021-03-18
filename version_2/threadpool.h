#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>
#include<signal.h>
#include<errno.h>

typedef struct{
	void *(*function)(void *);             //函数指针，回调函数
	void *arg;                          //回调函数的参数
}threadpool_task_t;                      //各子线程任务结构体

struct threadpool_t {
	pthread_mutex_t lock;          //互斥锁，用于锁住本结构体
	pthread_mutex_t thread_counter;  //记录忙线程个数的锁，busy_thr_num
	pthread_cond_t queue_not_full;   //条件变量，当任务队列满时，添加任务的线程阻塞，等待此条件变量
	pthread_cond_t queue_not_empty;   //条件变量，当任务队列不为空时，通知等待线程的任务

	pthread_t *threads;    //数组，存放线程池中每个线程的tid
	pthread_t adjust_tid;   //存放管理者线程的tid 
	threadpool_task_t *task_queue;  //任务队列

	int min_thr_num;    //线程池最小线程数
	int max_thr_num;    //线程池最大线程数
	int liv_thr_num;     //当前存活线程个数
	int busy_thr_num;    //忙状态线程个数
	int wait_exit_thr_num;  //要销毁的线程个数

	int queue_front;     //任务队列队头下标
	int queue_rear;      //任务队列队尾下标
	int queue_size;     //任务队列中实际任务数
	int queue_max_size;    //任务队列可容纳任务数上限
 
	int shutdown;       //标志位，线程池使用状态，true或false
};

threadpool_t *threadpool_create(int min_thr_num,int max_thr_num,int queue_max_size);  //创建线程池
void *threadpool_thread(void * threadpool);    //工作线程回调函数
void *adjust_thread(void *threadpool);     //管理者线程回调函数
int threadpool_add(threadpool_t *pool,void*(*function)(void *arg),void *arg);  //线程池添加任务
void *process(void *arg);    //任务函数
int is_thread_alive(pthread_t tid);   //测试线程是否存活
int threadpool_free(threadpool_t *pool);  //释放内存
int threadpool_destroy(threadpool_t *pool);   //销毁
