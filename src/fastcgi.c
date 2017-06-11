#include "fastcgi.h"
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>

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

/*
 * 连接php-fpm服务器
 * 成功返回socket描述符, 失败返回-1
 *
 */

int conn_fastcgi(){	
	int fcgi_fd;
	struct sockaddr_in fcgi_sock;

	memset(&fcgi_sock, 0, sizeof(fcgi_sock));
	fcgi_sock.sin_family = AF_INET;
	fcgi_sock.sin_addr.s_addr = inet_addr(FCGI_HOST);
	fcgi_sock.sin_port = htons(FCGI_PORT);
	
	if(-1 == (fcgi_fd = socket(PF_INET, SOCK_STREAM, 0))){
		err_sys("fcgi socket()", DEBUGPARAMS);
		return -1;
	}
	
	if(-1 == connect(fcgi_fd, (struct sockaddr *)&fcgi_sock, sizeof(fcgi_sock))){
		err_sys("php-fpm connect", DEBUGPARAMS);
		return -1;
	}
	return fcgi_fd;
}


/*
 * 与php-fpm通信
 * 成功返回1, 失败返回-1 * 
 * */
  
int send_fastcgi(int fcgi_fd, http_request *hr){
		
	char filename[50];
	int requestId = hr->sockfd, sendbytes;

	FCGI_BeginRequestRecord beginRecord;
	beginRecord.body = makeBeginRequestBody(FCGI_RESPONDER);
	beginRecord.header =  makeHeader(FCGI_BEGIN_REQUEST, requestId, sizeof(beginRecord.body), 0);
	if(-1 == (sendbytes = send(fcgi_fd, &beginRecord, sizeof(beginRecord), 0))){
		err_sys("fcgi send beginRecord error", DEBUGPARAMS);
		return -1;
	}

	getcwd(filename, sizeof(filename));
	strcat(filename, "/");
	strcat(filename, hr->path);
	// debug printf("%s\n", filename);
	 
	buffer_path_simplify(filename, filename);

	char method[10];
	char conlength[100];

	if(hr->method == POST) strcpy(method, "POST");
	else if(hr->method == GET) strcpy(method, "GET");
	sprintf(conlength, "%d", hr->conlength);
	char *params[][2] = {
		{"SCRIPT_FILENAME", filename}, 
		{"REQUEST_METHOD", method}, 
		{"QUERY_STRING", hr->param}, 
		{"CONTENT_TYPE", hr->contype },
		{"CONTENT_LENGTH", conlength },
		{"", ""}
	};

	//printf("%d  %s\n", hr->conlength, hr->content);
	int i, conLength, paddingLength;
	FCGI_ParamsRecord *paramsRecord;
	FCGI_Header emptyData;
	for(i = 0; strcmp(params[i][0],  "") != 0; i++){    
		//printf("%s : %s\n", params[i][0], params[i][1]);
		conLength = strlen(params[i][0]) + strlen(params[i][1]) + 2;
		paddingLength = (conLength % 8) == 0 ? 0 : 8 - (conLength % 8);
		paramsRecord = (FCGI_ParamsRecord *)malloc(sizeof(FCGI_ParamsRecord) + conLength + paddingLength);
		paramsRecord->nameLength = (unsigned char)strlen(params[i][0]);    // 填充参数值
		paramsRecord->valueLength = (unsigned char)strlen(params[i][1]);   // 填充参数名
		paramsRecord->header = makeHeader(FCGI_PARAMS, requestId, conLength, paddingLength);
		memset(paramsRecord->data, 0, conLength + paddingLength);
		memcpy(paramsRecord->data, params[i][0], strlen(params[i][0]));
		memcpy(paramsRecord->data + strlen(params[i][0]), params[i][1], strlen(params[i][1]));
		if(-1 == (sendbytes = send(fcgi_fd, paramsRecord, 8 + conLength + paddingLength, 0))){
			err_sys("fcgi send paramsRecord error", DEBUGPARAMS);
			return -1;
		}
		free(paramsRecord);
	}
	emptyData = makeHeader(FCGI_PARAMS, requestId, 0, 0);
	if(-1 == (sendbytes = send(fcgi_fd, &emptyData, FCGI_HEADER_LEN, 0))){
		err_sys("fcgi send paramsRecord empty error", DEBUGPARAMS);
		return -1;
	}

	char buf[8] = {0};
	int len = hr->conlength;
	//char content[1024];
	//strncpy(content, hr->content, len);
	//printf("总长度:%d %s\n", len, content);
	int send_len;
	if(hr->method == POST){
		//printf("post数据总长度:%d\n", len);
		//printf("len:%d\n", len);
		//puts(hr->content);
		while(len > 0){
			send_len = len > FCGI_MAX_LEN  ? FCGI_MAX_LEN : len;
			len -= send_len;
			FCGI_Header stdinHeader;
			paddingLength = (send_len %  8) == 0 ? 0 :  8 - (send_len % 8);
			stdinHeader = makeHeader(FCGI_STDIN, requestId, send_len, paddingLength);
			if(-1 == (sendbytes = send(fcgi_fd, &stdinHeader,FCGI_HEADER_LEN, 0))){
				err_sys("fcgi send stdinHeader error", DEBUGPARAMS);
				return -1;
			}
			if(-1 == (sendbytes = send(fcgi_fd, hr->content, send_len, 0))){
				err_sys("fcgi send stdin", DEBUGPARAMS);
				return -1;
			}
			//printf("单次发送大小:%d\n发送内容:\n%s\n", sendbytes, hr->content);
			if((paddingLength > 0) && (-1 == (sendbytes = send(fcgi_fd, buf, paddingLength, 0)))){
				err_sys("fcgi send stdin padding", DEBUGPARAMS);
				return -1;
			}
			hr->content += send_len;
			// debug printf("长度%d\n", (int)strlen(hr->content));
		}
	}

	//debug  printf("%d\n", sendbytes);
	FCGI_Header emptyStdin = makeHeader(FCGI_STDIN, requestId, 0, 0);
	if(-1 == (sendbytes = send(fcgi_fd, &emptyStdin, FCGI_HEADER_LEN, 0))){
		err_sys("fcgi send stdinHeader enpty", DEBUGPARAMS);
		return -1;
	}
	//printf("send ok\n");
	return 1;
}

 
void recv_fastcgi(int fcgi_fd, http_request *hr, http_response *rs){

	FCGI_Header responseHeader;
	FCGI_EndRequestBody responseEnder;
	int recvbytes, recvId, ok_recved = 0, ok_recv = 0;	// ok_recved 已经从缓冲区读取的字节数目,  ok_recv 本次要从缓冲区读取的字节数
	int err_recved = 0, err_recv = 0;					//同上
	char buf[8];
	int contentLength;
	int errlen = 0, outlen = 0;
	char *ok = NULL , *err = NULL;

	while(recv(fcgi_fd, &responseHeader, FCGI_HEADER_LEN, 0) > 0){
		recvId = (int)(responseHeader.requestIdB1 << 8) + (int)(responseHeader.requestIdB0);
		if(FCGI_STDOUT == responseHeader.type && recvId == hr->sockfd){
			contentLength = ((int)responseHeader.contentLengthB1 << 8) + (int)responseHeader.contentLengthB0;
			outlen += contentLength;
			if(ok != NULL){
				if(NULL == (ok = realloc(ok, outlen))){
					err_sys("realloc memory ok fail", DEBUGPARAMS);
					free(ok);
					return ;
				}
			}else{
				if(NULL == (ok = (char *)malloc(contentLength))){
					err_sys("alloc memory ok fail", DEBUGPARAMS);
					return ;
				}
			}
			while(contentLength > 0){
				//本次从缓冲区读取大小
				ok_recv = contentLength > MAX_RECV_SIZE ? MAX_RECV_SIZE : contentLength;
				if( -1 == (recvbytes = recv(fcgi_fd, ok + ok_recved, ok_recv, 0))){
					err_sys("fcgi recv stdout fail", DEBUGPARAMS);
					return ;
				}
				contentLength -= recvbytes;
				ok_recved += recvbytes;
			}	

			if(responseHeader.paddingLength > 0){
				recvbytes = recv(fcgi_fd, buf, responseHeader.paddingLength, 0);
				if(-1 == recvbytes || recvbytes != responseHeader.paddingLength){
					err_sys("fcgi stdout padding fail", DEBUGPARAMS);
					return;
				}
			}
		}else if(FCGI_STDERR == responseHeader.type && recvId == hr->sockfd){
			contentLength = ((int)responseHeader.contentLengthB1 << 8) + (int)responseHeader.contentLengthB0;
			errlen += contentLength;
			if(err != NULL){
				if( NULL == (err = realloc(err, errlen))){	
					err_sys("fcgi stderr realloc memory fail", DEBUGPARAMS);
					free(err);
					return ;
				}
			}else{
				if(NULL == (err = (char *)malloc(contentLength))){	
					err_sys("fcgi stderr alloc memory fail", DEBUGPARAMS);
					return ;
				}
			}
		
			while(contentLength > 0){
				//本次从缓冲区读取大小
				err_recv = contentLength > MAX_RECV_SIZE ? MAX_RECV_SIZE : contentLength;
				if( -1 == (recvbytes = recv(fcgi_fd, err + err_recved, err_recv, 0))){
					err_sys("fcgi recv stderr fail", DEBUGPARAMS);
					return ;
				}
				contentLength -= recvbytes;
				err_recved += recvbytes;
			}	

			if(responseHeader.paddingLength > 0){
				recvbytes = recv(fcgi_fd, buf, responseHeader.paddingLength, 0);
				if(-1 == recvbytes || recvbytes != responseHeader.paddingLength){
					err_sys("fcgi stdout padding", DEBUGPARAMS);
					return ;
				}
			}
		}else if(FCGI_END_REQUEST == responseHeader.type && recvId == hr->sockfd){
			recvbytes = recv(fcgi_fd, &responseEnder, sizeof(responseEnder), 0);
			
			if(-1 == recvbytes || sizeof(responseEnder) != recvbytes){
				free(err);
				free(ok);
				err_sys("fcgi recv end fail", DEBUGPARAMS);
				return ;
			}
			//printf("%s", ok);
			send_client(ok, outlen, err, errlen, hr, rs);
			free(err);
			free(ok);
		}
	}

}

