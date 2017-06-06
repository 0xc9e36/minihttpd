#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "../src/list.h" 

struct person { 
    int age; 
    char name[20];
    struct list_head list; 
};

int main(void) { 
    struct person *person_node; 
    struct person person_head; 
    struct list_head *pos, *next; 
    int i;

    // 初始化双链表的表头 
    INIT_LIST_HEAD(&person_head.list); 

    // 添加节点
    for ( i = 0; i < 5; i++){
        person_node = (struct person*)malloc(sizeof(struct person));
        person_node->age = (i+1)*10;
        sprintf(person_node->name, "用户%d", i+1);
        list_add_tail(&(person_node->list), &(person_head.list));
    }

    // 遍历链表
    printf("节点插入完毕, 开始遍历\n"); 
    list_for_each(pos, &person_head.list) { 
		/* 根据结构体成员获取结构体地址 */
        person_node = list_entry(pos, struct person, list); 
        printf("name:%-2s, age:%d\n", person_node->name, person_node->age); 
    } 
    
	/* 删除age为20的节点*/
	list_for_each_safe(pos, next, &person_head.list){
		person_node = list_entry(pos, struct person, list);
		if(20 == person_node->age){
			list_del_init(pos);
			free(person_node);
		}
	}

    // 遍历链表
    printf("删除完毕, 开始遍历\n"); 
    list_for_each(pos, &person_head.list) { 
        person_node = list_entry(pos, struct person, list); 
        printf("name:%-2s, age:%d\n", person_node->name, person_node->age); 
	}

	/* 释放内存 */
	list_for_each_safe(pos, next, &person_head.list){
		person_node = list_entry(pos, struct person, list);
		list_del_init(pos);
		free(person_node);
	}

	return 0;
}
