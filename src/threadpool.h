#ifndef _THREADPOOL_H_
#define _THREADPOOL_H

#include "common.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<pthread.h>

#define MAX_POOL_SIZE	4	//最大线程数

/* 工作队列 */
typedef struct job{
	void *(*process)(void *arg);
	void *arg;			
	struct job *next;
}thread_job;


/* 线程池 */
typedef struct{
	int is_destroy;
	pthread_mutex_t queue_lock;
	pthread_cond_t  queue_cond;
	int wait_num;
	int pool_size;
	thread_job *queue_head;
	pthread_t *tid;
}thread_pool;

static thread_pool *pool = NULL;

int pool_init(int pool_size);
void *thread_routine(void *arg);
int pool_destroy();
int add_job(void *(*process)(void *arg), void *arg);
void *test(void *arg);

#endif
