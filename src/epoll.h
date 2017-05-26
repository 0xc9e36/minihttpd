#ifndef _EPOLL_H_
#define _EPOLL_H_

#include "common.h"
#include<sys/epoll.h>

#define MAX_FD 1024
struct epoll_event *events;


int epoll_init(int flags);

#endif
