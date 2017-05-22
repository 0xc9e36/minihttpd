#include "rio.h"
#include "common.h"
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>

/* 
 *
 * 从fd中读取n个字节数据到usrbuf中
 * 成功返回读取的字节数
 * 失败返回-1
 *
 * */
ssize_t rio_readn(int fd, void *usrbuf, size_t n){
	size_t nleft = n;
	ssize_t nread;
	char *bufp = usrbuf;

	while(nleft > 0){
		if((nread = read(fd, bufp, nleft)) < 0){
			if(errno == EINTR){
				nread = 0;
			}else{
				return -1;
			}
		}else if(nread == 0){
			break;
		}
		nleft -= nread;
		bufp += nread;
	}
	return n - nleft;
}

/*
 * 
 * 将usrbuf中的n个字节写入fd
 * 成功返回写入的字节数
 * 失败返回-1
 *
 */
ssize_t rio_writen(int fd, void *usrbuf, size_t n){

	size_t nleft = n;
	ssize_t nwritten;
	char *bufp = usrbuf;

	while(nleft > 0){
		if((nwritten = write(fd, bufp, nleft)) <= 0){
			if(errno == EINTR){
				nwritten = 0;
			}else{
				return -1;
			}
		}
		nleft -= nwritten;
		bufp += nwritten;
	}
}

void rio_readinitb(rio_t *rp, int fd){
	rp->rio_fd = fd;
	rp->rio_cnt = 0;
	rp->rio_bufptr = rp->rio_buf;
}


/* 实现带缓冲的write操作 */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n){
	int cnt;

	/* 没有未读的先调用write()填充*/
	while(rp->rio_cnt <= 0){
		rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
		if(rp->rio_cnt < 0){
			if(errno != EINTR){
				return -1;
			}
		}else if(rp->rio_cnt == 0){
			return 0;
		}else{
			rp->rio_bufptr = rp->rio_buf;
		}
	}

	cnt = n;
	if(rp->rio_cnt < 0) cnt = rp->rio_cnt;
	memcpy(usrbuf, rp->rio_bufptr, cnt);
	rp->rio_bufptr += cnt;
	rp->rio_cnt -= cnt;
	return cnt;
}


/* 复制一行内容到usrbuf中. 最多maxlen - 1个字符, 最后一个字符用NULL填充*/
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen){
	
	int n, rc;
	char c, *bufp = usrbuf;
	
	for(n = 1; n < maxlen; n++){
		if((rc = rio_read(rp, &c, 1)) == 1){
			*bufp++ = c;
			if(c == '\n'){
				n++;
				break;
			}
		}else if(rc == 0){
			if(n == 1) return 0;
			else break;
		}else{
			return -1;
		}
	}
	*bufp = 0;
	return n - 1;
}


/* 带缓冲的read() */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n){
	size_t nleft = n;
	ssize_t nread;
	char *bufp = usrbuf;

	while(nleft > 0){
		if((nread = rio_read(rp, bufp, nleft)) < 0){
			return -1;
		}else if(nread == 0){
			break;
		}
		nleft -= nread;
		bufp += nread;
	}
	return (n - nleft);
}


