#ifndef _HTTP_H_
#define _HTTP_H_
#include "fastcgi.h"
#include "common.h"
#include<stdio.h>
#include<string.h>
#include<dirent.h>
#include<pwd.h>
#include<unistd.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<ctype.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<pthread.h>
#include<netinet/in.h>

#define		PORT	8899
#define		MAX_QUE_CONN_NM	20
#define		MAX_BUF_SIZE	8096

/* http请求信息结构体, 这里只记录几个关键信息, 其他忽略 */
typedef struct http_head{
	char method[10];		//请求方式
	char path[256];			//文件路径
	char filename[256];		//请求文件名
	char version[10];		//HTTP协议版本
	char url[256];			//请求url
	char param[256];		//请求参数
	char contype[256];		//消息类型
	char conlength[16];		//消息长度
	char ext[10];			//文件后缀
}http_header;

int init_server();
void parse_request(const int, char *, http_header *);
int get_line(const int, char *, int);
void *handle_request(void *);
void send_http_responce(int, const int, const char *, const http_header *);
void get_http_mime(char *, char *);
void exec_static(int, http_header *, int);
void exec_php(int, http_header *);
void exec_dir(int, char *, http_header *);
int conn_fasrcgi();

#endif
