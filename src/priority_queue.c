#include "priority_queue.h"
#include "common.h"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>


/* 初始化 */
int init_heap(priority_queue *pq_obj, pq_compare compare, size_t size){
	
	if(!(pq_obj->pq = (void **)malloc( sizeof(void *) * (size + 1) ) )){
		err_sys("alloc memory fail", DEBUGPARAMS);
		return -1;
	}
	pq_obj->size = size + 1;
	pq_obj->last = 0;
	pq_obj->compare = compare;
	return 1;
}

/* 加入元素 */
int insert_heap(priority_queue *pq_obj,void *node){
	
	/* 扩容操作 */
	if(pq_obj->last + 1 == pq_obj->size){
		if(-1 == resize(pq_obj, pq_obj->size << 1)) return -1;
	}

	pq_obj->pq[++pq_obj->last] = node;

//	printf("第%d个元素加入\n", pq_obj->last);
	size_t k = pq_obj->last;
	
	/* 元素上浮*/
	while(k > 1 && pq_obj->compare(pq_obj->pq[k], pq_obj->pq[parent(k)])){
		//printf("%d\n", pq_obj->compare(pq_obj->pq[k], pq_obj->pq[parent(k)]));
		exch(pq_obj, k, parent(k));
		k = parent(k);
	}
	return 1;
}

/* 下沉, 维持最小堆的性质, 假设i的左右孩子都已经是最小堆. */
void min_heapify(priority_queue *pq_obj, size_t i){
	size_t min, N = pq_obj->last;
	while((i << 1) <= N){
		min = (i << 1);
		if(min < N && pq_obj->compare(pq_obj->pq[min + 1], pq_obj->pq[min])) min++;  
		if(pq_obj->compare(pq_obj->pq[i], pq_obj->pq[min]))	break;
		exch(pq_obj, i, min);
		i = min;
	}
}

/* 获取堆顶元素 */
void *min_heap(priority_queue *pq_obj){
	if(1 == is_empty_heap(pq_obj)) return NULL;
	return pq_obj->pq[1]; 
}

/* 删除堆顶元素 */
int del_min_heap(priority_queue *pq_obj){
	
	if(1 == is_empty_heap(pq_obj)) return 1;
	exch(pq_obj, 1, pq_obj->last);
	pq_obj->last--;
	min_heapify(pq_obj, 1);
	
	if((pq_obj->last > 0) && (pq_obj->last  <=  (pq_obj->size - 1) >> 2 ) ){
		if(-1 == resize(pq_obj, pq_obj->size >> 1)) return -1;
	}
	return 1;
}

/* 扩容 */
int resize(priority_queue *pq_obj, size_t size){

	if(size <= pq_obj->last){
		err_sys("size unreasonable", DEBUGPARAMS);
		return -1;
	}
	
	//  debug printf("最新容量 : %lu\n", size);
	void **new_pq;
	if((new_pq = (void **)malloc(sizeof(void *) * size)) == NULL){
		err_sys("alloc memory fail", DEBUGPARAMS);
		return -1;
	}

	memcpy(new_pq, pq_obj->pq, sizeof(void *) * (pq_obj->last + 1));
	free(pq_obj->pq);
	pq_obj->pq = new_pq;
	pq_obj->size = size;
	return 1;
}

/* 判断是否为空 */
int is_empty_heap(priority_queue *pq_obj){
	return pq_obj->last == 0 ? 1 : -1;
}

/* 交换两个元素 */
void exch(priority_queue *pq_obj, size_t i, size_t j){
	void *tmp = pq_obj->pq[i];
	pq_obj->pq[i] = pq_obj->pq[j];
	pq_obj->pq[j] = tmp;
}

int heap_size(priority_queue *pq_obj){
	return pq_obj->last;
}
