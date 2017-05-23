#include "threadpool.h"

void *pthread_routine(void *arg){

	//debug  printf("starting pthread 0x%x\n", (unsigned int)pthread_self());

	pool->pool_size++;
	while(1){
		pthread_mutex_lock(&(pool->queue_lock));
	
		while(0 == pool->wait_num && ! pool->is_destroy){
	//debug printf("thread 0x%x waiting\n", (unsigned int)pthread_self());
			pthread_cond_wait(&(pool->queue_cond), &(pool->queue_lock));
		}

		if(pool->is_destroy){
			pthread_mutex_unlock(&(pool->queue_lock));
	 printf("thread 0x%x will exit\n", (unsigned int)pthread_self());
			pthread_exit(NULL);
		}

	//debug printf("pthread 0x%x is starting to work\n", (unsigned int)pthread_self());
		
		pool->wait_num--;
		thread_job *job = pool->queue_head;
		pool->queue_head = job->next;
		
		pthread_mutex_unlock(&(pool->queue_lock));
		(*(job->process))(job->arg);
		free(job);
		job = NULL;
	}

	// debug printf("excute here\n");
	pthread_exit(NULL);
}

/* 初始化线程池, 成功返回1, 失败返回-1 */
int pool_init(int pool_size){
	
	if(pool_size <= 0) {
		err_user("threadpool size must > 0\n");
		return -1;
	}
	

	if(NULL == (pool = (thread_pool *)malloc(sizeof(thread_pool)))){
		err_sys("threadpool alloc memory fail", DEBUGPARAMS);
		return -1;
	}
	
	if(0 != pthread_mutex_init(&(pool->queue_lock), NULL)){
		free(pool);
		err_sys("pthread_mutex_init fail", DEBUGPARAMS);
		return -1;
	}

	if(0 != pthread_cond_init(&(pool->queue_cond), NULL)){
		pthread_mutex_destroy(&(pool->queue_lock));
		free(pool);
		err_sys("pthread_cond_init fail", DEBUGPARAMS);
		return -1;
	}

	if(NULL == (pool->tid = (pthread_t *)malloc(pool_size * sizeof(pthread_t)))){
		pthread_mutex_destroy(&(pool->queue_lock));
		pthread_cond_destroy(&(pool->queue_cond));
		free(pool);
		err_sys("pthread_t alloc memory fail", DEBUGPARAMS);
		return -1;
	}
	pool->wait_num = 0;
	pool->pool_size = 0;
	pool->is_destroy = 0;
	pool->queue_head = NULL; 

	int i = 0;
	for(; i < pool_size; i++){
		if(0 != pthread_create(&(pool->tid[i]), NULL, pthread_routine, NULL)){
			pool_destroy();
			err_sys("pthread_create fail", DEBUGPARAMS);
			return -1;
		}
	}
	
	/* 等待所有线程创建完毕 */
	while(pool_size != pool->pool_size){}
	return 1;
}

/*
 *	销毁线程池
 *	失败返回 -1, 成功返回 1
 */

int pool_destroy(){
	if(pool->is_destroy) return -1;
	pool->is_destroy = 1;

	pthread_mutex_lock(&(pool->queue_lock));
	pthread_cond_broadcast(&(pool->queue_cond));
	pthread_mutex_unlock(&(pool->queue_lock));

	int i = 0;
	for(;  i < pool->pool_size; i++){
		pthread_join(pool->tid[i], NULL);
	}

	thread_job *head = NULL;

	while(pool->queue_head != NULL){
		head = pool->queue_head;
		pool->queue_head = head->next;
		free(head);
	}
	free(pool->tid);
	pthread_mutex_destroy(&(pool->queue_lock));
	pthread_cond_destroy(&(pool->queue_cond));
	free(pool);
	pool = NULL;
	return 1;
}

/*
 *
 * 添加到任务队列
 * 成功返回1, 失败返回-1
 *
 */

int add_job(void *(*process)(void *arg), void *arg){
	thread_job *newjob;
	if(NULL == (newjob = (thread_job *)malloc(sizeof(thread_job)))){
		err_sys("add_job alloc memory fail", DEBUGPARAMS);
		return -1;
	}
	newjob->process = process;
	newjob->arg = arg;
	newjob->next = NULL;

	
	pthread_mutex_lock(&(pool->queue_lock));
	thread_job *head = pool->queue_head;
	
	if(head != NULL){
		while(head->next != NULL) head = head->next;
		head->next = newjob;
	}else{
		pool->queue_head = newjob;
	}

	pool->wait_num++;
	pthread_mutex_unlock(&(pool->queue_lock));
	pthread_cond_signal(&(pool->queue_cond));
	
	return 1;

}

void *test(void *arg){
	printf("tid is 0x%x, working on job %d\n", (unsigned int)pthread_self(), *(int *)arg);
	return ;
}

