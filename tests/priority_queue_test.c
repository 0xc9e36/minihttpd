#include "../src/priority_queue.h"


struct person{
	size_t age;
	char name[50];
};

/* i < j 则返回 真 */
int compare(void *i, void *j){
	 
	struct person *a = (struct person *)i;
	struct person *b = (struct person *)j;
	return (a->age < b->age)? 1 : 0; 
	
}

int main(void){

	priority_queue pq_obj;

	int rs;
	rs = init_heap(&pq_obj, compare, DF_PQ_SIZE);
	if(!rs)	 exit(1);
	printf("初始化优先队列成功 : %d\n", rs);
	
	if(1 == is_empty_heap(&pq_obj)) printf("队列为空\n");

	//printf("队列大小 : %d\n", heap_size(&pq_obj));

	struct person person[10];

	int i, len = sizeof(person) / sizeof(struct person);
	for(i = 0; i < len; i++){
		person[i].age = 10 - (size_t)i;
		sprintf(person[i].name, "用户%d", i);
		insert_heap(&pq_obj, (void *)&person[i]);
	}


	//del_min_heap(&pq_obj);

	//printf("移除堆顶后大小 : %d\n", heap_size(&pq_obj));

	struct person  *min;

	while(-1 == is_empty_heap(&pq_obj)){
		min = (struct person *)min_heap(&pq_obj);		

		printf("删除堆顶元素 %lu, 剩余队列大小 %d\n",min->age, heap_size(&pq_obj));
		//printf("删除堆顶元素 %lu, 剩余队列大小 %d\n",min->age, heap_size(&pq_obj));
		rs = del_min_heap(&pq_obj);
	}

	//printf("队列大小 : %d\n", heap_size(&pq_obj));

	return 0;
}
