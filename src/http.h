#ifndef _HTTP_H_
#define _HTTP_H_
#include "fastcgi.h"
#include "common.h"
#include "epoll.h"
#include "list.h"
#include "threadpool.h"
#include<stdio.h>
#include<string.h>
#include<dirent.h>
#include<pwd.h>
#include<unistd.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/wait.h>
#include<ctype.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<pthread.h>
#include<netinet/in.h>

#define		PORT	8899
#define		MAX_QUE_CONN_NM	20
#define		MAX_BUF_SIZE	8096
#define		MAX_RECV_SIZE	4000

/* HTTP状态信息 */
#define		HTTP_LINE_ERROR	-1
#define		HTTP_HEADER_ERROR	0
#define		HTTP_BODY_ERROR	1


#define HTTP_OK	200
#define HTTP_AGAIN EAGAIN

#define GET		100
#define	POST	101
#define UNKNOW	102

/* http请求信息结构体, 这里只记录几个关键信息, 其他忽略 */
typedef struct http_request{
	size_t pos, last;
	char buf[MAX_BUF_SIZE];

	void *request_start;
	void *request_end;

	void *method_end;
	int method;

	void *path_start;
	void *path_end;
	char path[1024];
	
	char filename[1024];


	int http_major;
	int http_minor;
	char version[10];			//HTTP协议版本

	void *url_start;
	void *url_end;
	char url[1024];

	void *key_start;
	void *key_end;
	void *val_start;
	void *val_end;

	char *param_start;		//请求参数
	char *param_end;
	char param[1024];

	char ext[10];

	char *content_start;	//body
	char *content_end;
	int sockfd;				//socket描述符
	int epfd;				//事件
	int state;				//状态
	struct list_head list;	
}http_request;

/* http请求头信息 */
typedef struct http_header{

	void *key_start, *key_end;
	void *val_start, *val_end;
	list_head list;
}http_header;

/* http响应头 */
typedef struct{
	int sockfd;
	int keep_alive;
	time_t ctime;	//最后修改时间
	int modifyed;	//是否修改过

	int status;
}http_response;

int init_server();

void init_response(http_response *rs, int sockfd);
int set_non_blocking(int sockfd);
int init_http_request(http_request *, int, int);
int parse_request(http_request *);
int get_line(const int, char *, int);
void *handle_request(void *);
void send_http_responce(int, const int, const char *, const http_request *);
void get_http_mime(char *, char *);
void exec_static(http_request *, int);
void exec_php(char *, http_request *);
void exec_dir(int, char *, http_request *);
int conn_fastcgi();
int send_fastcgi(int ,char *, http_request *);
void recv_fastcgi(int ,int, http_request *);
char *get_http_Val(const char* str, const char *substr);
void send_client(char *, int, char *, int, int, http_request *);
void http_close(http_request *hr);
#endif
