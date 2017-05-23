#ifndef _PRIORITY_QUEUE__H_
#define _PRIORITY_QUEUE__H_

/*最小优先队列实现, 细节可参考算法导论第三版或算法第四版(红色封面)*/

#include "common.h"

#define left(i)		i << 1	
#define right(i)	i << 1 + 1
#define	parent(i)	i >> 1
#define	DF_PQ_SIZE	10

typedef int (*pq_compare)(void *i, void *j);

typedef struct {
	void **pq;							//节点数组		
	size_t last;						//当前容量
	size_t size;						//最大容量
	pq_compare compare;					//比较函数
}priority_queue;


int init_heap(priority_queue *pq_obj, pq_compare compare, size_t size);
int insert_heap(priority_queue *pq_obj,void *node);
void *min_heap(priority_queue *pq_obj);
int is_empty_heap(priority_queue *pq_obj);
void min_heapify(priority_queue *pq_obj, size_t i);
void exch(priority_queue *pq_obj, size_t i, size_t j);
int resize(priority_queue *pq_obj, size_t size);
int heap_size(priority_queue *pq_obj);
int del_min_heap(priority_queue *pq_obj);

#endif
