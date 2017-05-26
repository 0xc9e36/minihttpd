#ifndef _LIST_HEAD_H_
#define _LIST_HEAD_H_

/* 参考linux内核实现, 侵入式双链表, 十分巧妙, 更多参考大佬文章 : http://www.cnblogs.com/skywang12345/p/3562146.html */

typedef struct list_head{
	struct list_head *next, *prev;
}list_head;

/* 遍历链表 */
#define list_for_each(pos, head) \
	for(pos = (head)->next; pos != (head); pos = pos->next)

/* 遍历, 用在删除操作时 */
#define list_for_each_safe(pos, n, head) \
	for(pos = (head)->next, n = pos->next; pos != (head);	\
			pos = n, n = pos->next)

#define LIST_HEAD_INIT(name) {&(name), &(name)}

#define LIST_HEAD(name)\
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list){
	list->next = list;
	list->prev = list;
}

static inline void _list_add(struct list_head *new, struct list_head *prev, struct list_head *next){
	next->prev = new;
	new->next = next;
	prev->next = new;
	new->prev = prev;
}

/* 头插法 */
static inline void list_add(struct list_head *new, struct list_head *head){
	_list_add(new, head, head->next);
}

/* 尾插法 */
static inline void list_add_tail(struct list_head *new, struct list_head *head){
	_list_add(new, head->prev, head);
}

static inline void _list_del(struct list_head *prev, struct list_head *next){
	prev->next = next;
	next->prev = prev;
}

/* 删除节点 */
static inline void list_del(struct list_head *entry){
	_list_del(entry->prev, entry->next);
}

static inline void _list_del_entry(struct list_head *entry){
	_list_del(entry->prev, entry->next);
}

/* 删除节点, 并让节点的前驱和后继都指向自己 */
static inline void list_del_init(list_head *entry){
	_list_del_entry(entry);
	INIT_LIST_HEAD(entry);
}

/* 判断双链表是否为空 */
static inline int list_empty(const struct list_head *head){
	return head->next == head;
}

/* 获取MEMBER在结构体TYPE中的偏移 */
#define offsetof(type, member) ((size_t) &((type *)0)->member)

/* 根据域成员变量(member)的指针(ptr)获取整个结构体(type)变量指针 */
/* 定义了一个不可变的_mpt指针, 指向的内容是type结构体中的member成员地址. 然后用_mptr地址减去_mptr指针的偏移就得到了type的地址 */
#define container_of(ptr, type, member) ({	\
		const typeof( ((type *)0)->member) * _mptr = (ptr);	\
		(type *)( (char *)_mptr - offsetof(type, member) );})

#define list_entry(ptr, type, member)	\
		container_of(ptr, type, member)

#endif


