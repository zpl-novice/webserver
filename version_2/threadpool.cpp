#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>
#include<signal.h>
#include<errno.h>
#include "threadpool.h"

#define DEFAULT_TIME 10   //每10s检测一次
#define MIN_WAIT_TASK_NUM 10    //如果queue_size>10,添加新的线程到线程池
#define DEFAULT_THREAD_VARY 10   //每次创建和销毁线程的个数
#define true 1
#define false 0

threadpool_t *threadpool_create(int min_thr_num,int max_thr_num,int queue_max_size){
	int i;
	threadpool_t *pool = NULL;
	do{
		if((pool = (threadpool_t *)malloc(sizeof(threadpool_t)))==NULL){
			printf("malloc threadpool fail");
			break;                  //结构体申请内存失败，跳出do while循环
		}
		
		pool->min_thr_num = min_thr_num;
		pool->max_thr_num = max_thr_num;
		pool->busy_thr_num = 0;                  //初始时忙状态线程为0
		pool->liv_thr_num = min_thr_num;           //存活线程数，初值为最小线程数
		pool->queue_size = 0;                  //初始任务队列为0
		pool->queue_max_size = queue_max_size;
		pool->queue_front = 0;
		pool->queue_rear = 0;
		pool->shutdown = false;            //不关闭线程池

		//根据最大线程上限数，给工作线程数组开辟空间，并清零
		pool->threads =(pthread_t *)malloc(sizeof(pthread_t)*max_thr_num);
		if(pool->threads == NULL){
			printf("malloc threads fail");              //开辟空间失败，跳出循环
			break;
		}
		memset(pool->threads,0,sizeof(pthread_t)*max_thr_num);
	
		//任务队列开辟空间
		pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t)*queue_max_size);
		if(pool->task_queue == NULL){
			printf("malloc threads fail");              //开辟空间失败，跳出循环
			break;
		}
		
		//初始化互斥锁、条件变量
		if(pthread_mutex_init(&(pool->lock),NULL) !=0 || pthread_mutex_init(&(pool->thread_counter),NULL) !=0 || pthread_cond_init(&(pool->queue_not_empty),NULL) !=0  || pthread_cond_init(&(pool->queue_not_full),NULL) !=0){
			printf("init the lock or cond fail");              //初始化互斥锁、条件变量失败，跳出循环
			break;
		}

		//创建并启动min_thr_num个工作线程,调用对应的回调函数
		for(i=0;i<min_thr_num;i++){
			pthread_create(&(pool->threads[i]), NULL , threadpool_thread,(void *)pool); //pool指向当前线程池
		}

		//创建管理者线程,调用对应的回调函数
		pthread_create(&(pool->adjust_tid),NULL,adjust_thread,(void *)pool);
		return pool;
	}while(0);    //  只执行一次
	threadpool_free(pool);    //前面调用失败时会执行该句，释放空间
	return NULL;
}

void *threadpool_thread(void *threadpool){        //每个工作线程的回调函数
	threadpool_t *pool = (threadpool_t *)threadpool;
	threadpool_task_t task;
	
	while(true){
		pthread_mutex_lock(&(pool->lock));
		//没有任务，即queue_size==0时，调wait阻塞在条件变量上，若有任务，跳过该while
		while((pool->queue_size == 0) && (!pool->shutdown)){
			pthread_cond_wait(&(pool->queue_not_empty),&(pool->lock));
			//清除指定数目的空闲线程，如果要结束的线程个数大于0，结束线程
			if(pool->wait_exit_thr_num>0){
				pool->wait_exit_thr_num--;
				//如果线程池中的线程个数大于最小值就可以结束当前线程
				if(pool->liv_thr_num>pool->min_thr_num){
					pool->liv_thr_num--;
					pthread_mutex_unlock(&(pool->lock));
					pthread_exit(NULL);
				}
			}
		}
		if(pool->shutdown){
			pthread_mutex_unlock(&(pool->lock));
			pthread_exit(NULL);
		}
		//从任务队列中获取任务，出队操作
		task.function = pool->task_queue[pool->queue_front].function;
		task.arg = pool->task_queue[pool->queue_front].arg;
		//出队，模拟环形队列
		pool->queue_front = (pool->queue_front+1)%pool->queue_max_size;
		pool->queue_size--;

		//通知有新的任务可以添加进来
		pthread_cond_broadcast(&(pool->queue_not_full));

		//任务取出后，将线程池锁释放
		pthread_mutex_unlock(&(pool->lock));

		//执行任务
		pthread_mutex_lock(&(pool->thread_counter));   //忙状态线程数锁
		pool->busy_thr_num++;                   //忙状态线程数+1
		pthread_mutex_unlock(&(pool->thread_counter));   //忙状态线程数锁
		(*(task.function))(task.arg);      //执行任务回调函数

		//任务处理结束
		pthread_mutex_lock(&(pool->thread_counter));   //忙状态线程数锁
		pool->busy_thr_num--;                   //忙状态线程数-1
		pthread_mutex_unlock(&(pool->thread_counter));   //忙状态线程数锁
	}
	pthread_exit(NULL);
}

void *adjust_thread(void *threadpool){         //管理者线程的回调函数
	int i;
	threadpool_t *pool = (threadpool_t *)threadpool;
	while(!pool->shutdown){
		sleep(DEFAULT_TIME);      //定时对线程池管理
		pthread_mutex_lock(&(pool->lock));
		int queue_size = pool->queue_size;
		int liv_thr_num = pool->liv_thr_num;   //关注任务数和存活线程数
		pthread_mutex_unlock(&(pool->lock));

		pthread_mutex_lock(&(pool->thread_counter));   //忙状态线程数锁
		int busy_thr_num = pool->busy_thr_num;                   //忙状态线程数
		pthread_mutex_unlock(&(pool->thread_counter));   //忙状态线程数锁

		//创建新线程算法：任务数大于最小线程池个数，且存活的线程数少于最大线程个数时，如30>=10 && 40<100
		if(queue_size >= MIN_WAIT_TASK_NUM && liv_thr_num < pool->max_thr_num){
			pthread_mutex_lock(&(pool->lock));
			int add=0;
			//一次增加DEFAULT_THREAD_VARY个线程
			for(i=0;i < pool->max_thr_num && add< DEFAULT_THREAD_VARY && pool->liv_thr_num<pool->max_thr_num;i++){
				if(pool->threads[i] ==0 || !is_thread_alive(pool->threads[i])){
					pthread_create(&(pool->threads[i]),NULL,threadpool_thread,(void *)pool);
					add++;
					pool->liv_thr_num++;
				}
			}
			pthread_mutex_unlock(&(pool->lock));
		}

		//销毁多余的线程算法：忙线程x2小于存活的线程数且存活的线程数大于最小线程数时
		if((busy_thr_num*2)<liv_thr_num && liv_thr_num > pool->min_thr_num){
			pthread_mutex_lock(&(pool->lock));
			pool->wait_exit_thr_num=DEFAULT_THREAD_VARY;
			pthread_mutex_unlock(&(pool->lock));
			//通知处在空闲状态的线程，它们会自行终止
			for(i=0;i<DEFAULT_THREAD_VARY;i++){
				pthread_cond_signal(&(pool->queue_not_empty));
			}
		}
	}
	return NULL;
}

int threadpool_add(threadpool_t *pool,void*(*function)(void *arg),void *arg){   //向线程池添加任务函数
	pthread_mutex_lock(&(pool->lock));
	//队列满就阻塞
	while((pool->queue_size == pool->queue_max_size) && (!pool->shutdown)){
		pthread_cond_wait(&(pool->queue_not_full),&(pool->lock));
	}
	if(pool->shutdown){
		pthread_mutex_unlock(&(pool->lock));
	}
	//清空工作线程调用的回调函数的参数arg
	if(pool->task_queue[pool->queue_rear].arg !=NULL){
		free(pool->task_queue[pool->queue_rear].arg);
		pool->task_queue[pool->queue_rear].arg = NULL;
	}
	//添加任务到任务队列里
	pool->task_queue[pool->queue_rear].function = function;
	pool->task_queue[pool->queue_rear].arg = arg;
	pool->queue_rear = (pool->queue_rear+1)%pool->queue_max_size;  //队尾指针移动，模拟环形队列
	pool->queue_size++;

	//添加完任务后，队列不为空，唤醒线程池中等待处理任务的线程
	pthread_cond_signal(&(pool->queue_not_empty));
	pthread_mutex_unlock(&(pool->lock));
	return 0;
}

int threadpool_destroy(threadpool_t *pool){
	int i;
	if(pool == NULL){
		return -1;
	}
	pool->shutdown = true;
	//先销毁管理线程
	pthread_join(pool->adjust_tid,NULL);
	for(i-0;i<pool->liv_thr_num;i++){
		pthread_cond_broadcast(&(pool->queue_not_empty));
	}
	for(i=0;i<pool->liv_thr_num;i++){
		pthread_join(pool->threads[i],NULL);
	}
	threadpool_free(pool);
	
	return 0;
}

int threadpool_free(threadpool_t *pool){
	if(pool==NULL){
		return -1;
	}
	if(pool->task_queue){
		free(pool->task_queue);
	}
	if(pool->threads){
		free(pool->threads);
		pthread_mutex_lock(&(pool->lock));
		pthread_mutex_destroy(&(pool->lock));
		pthread_mutex_lock(&(pool->thread_counter));
		pthread_mutex_destroy(&(pool->thread_counter));
		pthread_cond_destroy(&(pool->queue_not_empty));
		pthread_cond_destroy(&(pool->queue_not_full));

	}
	free(pool);
	pool=NULL;
	return 0;
}

int is_thread_alive(pthread_t tid){
	int kill_rc =pthread_kill(tid,0);           //发0号信号，测试线程是否存活
	if(kill_rc == ESRCH){
		return false;
	}
	return true;
}










































