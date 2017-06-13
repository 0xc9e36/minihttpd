#ifndef _HTTP_INFO_H_
#define _HTTP_INFO_H_


#include "common.h"
#include "list.h"


#define		PORT				8899
#define		MAX_QUE_CONN_NM		20
#define		HTTP_BUF_SIZE		200000	//http接收缓冲区大小
#define		MAX_BUF_SIZE		8096
#define		MAX_RECV_SIZE		4000

#define		HTTP_LINE_ERROR		-3			//解析请求行错误
#define		HTTP_HEADER_ERROR	-2			//解析请求头错误
#define		HTTP_BODY_ERROR		-1			//解析请求体错误


#define HTTP_AGAIN EAGAIN

#define GET		100
#define	POST	101
#define UNKNOW	102

/* http状态码 */
#define		HTTP_OK						200
#define		HTTP_NOT_MODIFIED			304
#define		HTTP_FORBIDDEN				403
#define		HTTP_NOT_FOUND				404
#define		HTTP_NOT_IMPLEMENTED		501
#define		HTTP_VERSION_NOT_SUPPORTED	505


/* http请求信息结构体, 这里只记录几个关键信息, 其他忽略 */
typedef struct http_request{

	size_t pos, last;

	char buf[HTTP_BUF_SIZE];		//缓冲区

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
	char version[10];		//HTTP协议版本

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

	int alalyzed;			//已经解析的请求体 初始为0 

	char *content;	//body
	int conlength;
	int read_length;		//已经读取的长度

	char contype[255];		//读取请求类型

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
	int keep_alive;	//保持连接
	time_t ctime;	//最后修改时间
	int modified;	//是否修改过

	int status;
}http_response;

typedef int (*handle_fun)(http_request *hr, http_response *response, char *val, int len);

typedef struct{
	char *key;
	handle_fun  handle;
}header_handle;


#endif
