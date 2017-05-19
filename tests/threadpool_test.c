#include "../src/threadpool.h"

int main(void){
	
	pool_init(MAX_POOL_SIZE);
	int *jobs = (int *)malloc(20 * sizeof(int));
	int i = 0;
	for(; i < 20; i++){
		jobs[i] = i;
		add_job(test, &jobs[i]);
	}

	sleep(5);
	//仅当正在执行任务的线程完成后就会退出
	pool_destroy();
	free(jobs);
	return 0;
}
