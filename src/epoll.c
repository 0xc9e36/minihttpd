#include "epoll.h"


/* 初始化epoll, 成功返回epfd */
int epoll_init(int flags){
	int epfd;

	if((epfd = epoll_create1(flags)) < 0 ){
		err_sys("epoll_create fail", DEBUGPARAMS);
		return -1;
	}
	
	if(!(events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * MAX_FD))){	
		err_sys("alloc events memory fail", DEBUGPARAMS);
		return -1;
	}

	return epfd;
}
