#include "fastcgi.h"

FCGI_Header makeHeader(int type, int requestId, int contentLength, int paddingLength){
	FCGI_Header fastcgi_header;
	fastcgi_header.version = FCGI_VERSION_1;
	fastcgi_header.type = (unsigned char)type;	
    fastcgi_header.requestIdB1      = (unsigned char) ((requestId     >> 8) & 0xff);
	fastcgi_header.requestIdB0      = (unsigned char) ((requestId         ) & 0xff);
	fastcgi_header.contentLengthB1  = (unsigned char) ((contentLength >> 8) & 0xff);
	fastcgi_header.contentLengthB0  = (unsigned char) ((contentLength     ) & 0xff);
	fastcgi_header.paddingLength = (unsigned char)paddingLength;
	fastcgi_header.reserved = 0;
	return fastcgi_header;	
}

FCGI_BeginRequestBody makeBeginRequestBody( int role){
	FCGI_BeginRequestBody fastcgi_body;
	fastcgi_body.roleB1 = (unsigned char) ((role >>  8) & 0xff);
	fastcgi_body.roleB0 = (unsigned char) (role         & 0xff);
	fastcgi_body.flags  = (unsigned char) 0;	//关闭线路
	memset(fastcgi_body.reserved, 0, sizeof(fastcgi_body.reserved));
	return fastcgi_body;
}


