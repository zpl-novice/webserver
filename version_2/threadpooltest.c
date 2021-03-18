#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>
#include<signal.h>
#include<errno.h>

#include"threadpool.h"

void *process(void *arg){
	printf("thread 0x%x working on task %d\n ",(unsigned int)pthread_self(),(long)arg);
	sleep(1);
	printf("task %d is end\n ",(long)arg);

	return NULL;
}

int main(){
	threadpool_t *thp=threadpool_create(3,100,100); //创建线程池，池里最小3个线程，最大100，队列最大100
	printf("pool inited");

	int num[20],i;
	for(i=0;i<20;i++){
		num[i]=i;
		printf("add task %d\n",i);
		threadpool_add(thp,process,(void *)&num[i]);   //向线程池添加任务
	}
	sleep(10);
	threadpool_destroy(thp);
	return 0;
}
