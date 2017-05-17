
/* 详细文档 : https://fossies.org/dox/FCGI-0.78/fastcgi_8h.html */

#ifndef _FASTCGI_H_
#define _FASTCGI_H_

#include<stdio.h>
#include<string.h>

#define FCGI_VERSION_1           1
#define	FCGI_PORT				 9000
#define	FCGI_HOST				 "127.0.0.1"
#define FCGI_MAX_LEN		     65536

/*fastcgi报文协议头 */
typedef struct{
	unsigned char version;
	unsigned char type;
	unsigned char requestIdB1;
	unsigned char requestIdB0;
	unsigned char contentLengthB1;
	unsigned char contentLengthB0;
	unsigned char paddingLength;
	unsigned char reserved;
}FCGI_Header;

#define	FCGI_HEADER_LEN	 8

/* 可用于type字段的值 */
#define FCGI_BEGIN_REQUEST  1
#define FCGI_ABORT_REQUEST  2
#define FCGI_END_REQUEST    3
#define FCGI_PARAMS         4
#define FCGI_STDIN          5
#define FCGI_STDOUT         6
#define FCGI_STDERR         7
#define FCGI_DATA           8


/* 开始请求报文协议体 */
typedef struct{
	unsigned char roleB1;
	unsigned char roleB0;
	unsigned char flags;
	unsigned char reserved[5];
}FCGI_BeginRequestBody;

/* 期望php-fpm扮演的角色 */
#define FCGI_RESPONDER  1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER     3


/* 开始请求报文结构 */
typedef struct{
	FCGI_Header header;
	FCGI_BeginRequestBody body;
}FCGI_BeginRequestRecord;


typedef struct{
	FCGI_Header header;
	unsigned char nameLength;
	unsigned char valueLength;
	unsigned char data[0];
}FCGI_ParamsRecord;

typedef struct {
	unsigned char appStatusB3;
	unsigned char appStatusB2;
	unsigned char appStatusB1;
	unsigned char appStatusB0;
	unsigned char protocolStatus;   // 协议级别的状态码
	unsigned char reserved[3];
} FCGI_EndRequestBody;
/* 结束请求报文结构 */
typedef struct {
	FCGI_Header header;
	FCGI_EndRequestBody body;
} FCGI_EndRequestRecord;


FCGI_Header makeHeader(int type, int requestId, int contentLenth, int paddingLength);
FCGI_BeginRequestBody makeBeginRequestBody(int role);

#endif
