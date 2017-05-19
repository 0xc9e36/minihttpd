#include "common.h"
#include "http.h"
#include "fastcgi.h"

/*
 * 初始化socket
 * 成功返回socket描述符, 失败返回-1
 */
int init_server(){
	int sockfd;
	struct sockaddr_in server_sock;
	if(-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))){
		err_sys("socket() fail", DEBUG);
		return -1;
	}
	printf("Socket id = %d\n", sockfd);

	server_sock.sin_family = AF_INET;
	server_sock.sin_port = htons(PORT);
	server_sock.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_sock.sin_zero), 8);

	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	
	if(-1 == bind(sockfd, (struct sockaddr *)&server_sock, sizeof(struct sockaddr))){
		err_sys("bind() fail", DEBUG);
		return -1;
	}
	printf("Bind success\n");

	if(-1 == listen(sockfd, MAX_QUE_CONN_NM)){
		err_sys("listen() fail", DEBUG);
		return -1;
	}
	printf("Listening port = %d\n", PORT);
	
	return sockfd;
}

void *handle_request(void *arg){
	struct stat st;
	int client_fd;
	int recvbytes;
	char buf[MAX_BUF_SIZE];
	http_header hr;

	client_fd = *(int *)arg;
	memset(&hr, 0, sizeof(hr));
	memset(&buf, 0, sizeof(buf));

	/* 解析url, 提取关键请求信息 */
	if(-1 == parse_request(client_fd, buf, &hr)) return ;
	
	/* 只支持 GET 和 POST请求 */
	if(strcasecmp(hr.method, "GET") && strcasecmp(hr.method, "POST")){
		//501未实现
		send_http_responce(client_fd, 501, "Not Implemented", &hr);
		close(client_fd);
		return ;
	}
	/* 简单判断HTTP协议 */
	if(NULL == strstr(hr.version, "HTTP/")){
		// 505协议版本不支持
		send_http_responce(client_fd, 505, "HTTP Version Not Supported", &hr);
		close(client_fd);
		return;
	}
	
	/* 访问文件 */
	if(-1 == stat(hr.path, &st)){
		/* 404文件未找到 */
		send_http_responce(client_fd, 404, "Not Found", &hr);
	}else{
		//目录浏览
		if((st.st_mode & S_IFMT) == S_IFDIR){
			exec_dir(client_fd, hr.path, &hr);
		}else{	
			/* 提取文件类型 */
			char *path;
			path = strrchr(hr.path, '.');
			strcpy(hr.ext, (path + 1));

			if(0 == strcmp(hr.ext, "php")){	//php 文件
				if(!S_ISREG(st.st_mode) || !(S_IXUSR & st.st_mode)) {
					// 403 无执行权限
					send_http_responce(client_fd, 403, "Forbidden", &hr);
				}else{
					//执行php  -> 调用php-fpm
					exec_php(client_fd, &hr);
				}
			}else{	//静态文件	
				if(!S_ISREG(st.st_mode) || !(S_IRUSR & st.st_mode)) {
					// 403 无读权限
					send_http_responce(client_fd, 403, "Forbidden", &hr);
				}else{
					/* html静态页, jpg, png, pdf ...   */
					exec_static(client_fd, &hr, st.st_size);
				}
			}
		}
	}

	close(client_fd);	
}


/* 解析HTTP请求信息 */
int parse_request(const int client_fd, char *buf, http_header *hr){

	char web[50];
	int recvbytes;
	int i = 0, j = 0;
	int start, end;

	/* 根目录 */
	sprintf(web, "%s%s", ROOT, WEB);
	
	/* http 请求行 */
	recvbytes = get_line(client_fd, buf, MAX_BUF_SIZE);

	/* 请求方法 */
	while((i < recvbytes) && !isspace(buf[i])){
		hr->method[i] = buf[i];
		i++;
	}
	hr->method[i++] = '\0';

	/* 请求url */
	while((i < recvbytes) && !isspace(buf[i])){
		hr->url[j++] = buf[i++];
	}
	hr->url[j] = '\0';
	i++;

	j = 0;
	/* 协议版本 */
	while((i < recvbytes) && !isspace(buf[i])){
		hr->version[j++] = buf[i++];
	}
	hr->version[j] = '\0';

	/*  请求参数 */
	if(strstr(hr->url, "?")){
		strcpy(hr->filename, strtok(hr->url, "?"));
		strcpy(hr->param, strtok(NULL, "?"));
	}else{
		strcpy(hr->filename, hr->url);
		strcpy(hr->param, "");
	}

	/* 文件路径 */
	sprintf(hr->path, "%s%s", web, hr->filename);

	char *val;
	/*Content-Type以及Content-Length */
	while((recvbytes > 0) && strcmp("\n", buf)){
		recvbytes = get_line(client_fd, buf, MAX_BUF_SIZE);
		if((val = get_http_Val(buf, "Content-Length")) != NULL){
			strcpy(hr->conlength, val);	
		}else if((val = get_http_Val(buf, "Content-Type")) != NULL){
			strcpy(hr->contype, val);
		}
	}

	/* body内容  recv不能一次性接受很大值...  或者可以setsockopt 增大接受缓冲区*/
	int len = atoi(hr->conlength);
	if(len > 0 && strcmp("POST", hr->method) == 0){	
		if((hr->content = (char *)malloc(len)) == NULL){
			err_sys("http body alloc memory fail", DEBUG);
			return -1;
		}
		char *cur_recv = hr->content;
		while(len > 0){
			recvbytes =  recv(client_fd, cur_recv, MAX_RECV_SIZE, 0);
			if(-1 == recvbytes){
				err_sys("recv http body fail", DEBUG);
				return -1;
			}
			cur_recv += recvbytes;
			len -= recvbytes;
		}
		// debug printf("recv body中的内容是%d\n");
	}
	return 1;
}


char *get_http_Val(const char* str, const char *substr){
	char *start;
	if(strstr(str, substr) == NULL) return NULL;
	start = index(str, ':');
	//去掉空格冒号
	start += 2;
	//去掉换行符
	start[strlen(start) - 1] = '\0';
	return start;
}

/*  获取http请求一行数据, 成功则返回数据长度*/
int get_line(const int client_fd, char *buf, int size){
	char c = '\0';
	int n, i;
	for(i = 0; (c != '\n') &&  (i < size - 1); i++){
		n = recv(client_fd, &c, 1, 0);
		if(n > 0){
			if('\r' == c){
				n = recv(client_fd, &c, 1, MSG_PEEK);
				if((n > 0) && ('\n' == c)){
					recv(client_fd, &c, 1, 0);
				}else{
					c = '\n';
				}
			}
			buf[i] = c;
		}else{
			c = '\n';
		}
	}
	buf[i] = '\0';
	return i;
}

/* 发送http响应信息 */
void send_http_responce(int client_fd, const int http_code, const char *msg, const http_header * hr){
	char header[MAX_BUF_SIZE], body[MAX_BUF_SIZE];
	sprintf(body, "<html><body>%d:%s<hr /><em>studyHttpd Web Service</em></body></html>", http_code, msg);
	sprintf(header, "%s %d %s\r\n",hr->version, http_code, msg);
	send(client_fd, header, strlen(header), 0);
	sprintf(header, "Content-Type: text/html\r\n");
	send(client_fd, header, strlen(header), 0);
	sprintf(header, "Content-Length: %d\r\n\r\n", (int)strlen(body));
	send(client_fd, header, strlen(header), 0);
	send(client_fd, body, strlen(body), 0);
}

/* 处理静态文件 */
void exec_static(int client_fd, http_header *hr, int size){
	FILE *fp;
	int sendbytes;
	char header[MAX_BUF_SIZE];
	char body[MAX_BUF_SIZE];
	char mime[MAX_BUF_SIZE];

	fp = fopen(hr->path, "rb");
	
	if(NULL == fp){
		send_http_responce(client_fd, 404, "Not Found", hr);
		return ;
	}
	/* 获取文件MIME类型 text/html image/png ... */
	get_http_mime(hr->ext, mime);

	sprintf(header, "%s 200 OK\r\n",hr->version);
	send(client_fd, header, strlen(header), 0);
	sprintf(header, "Content-Type: %s\r\n", mime);
	send(client_fd, header, strlen(header), 0);
	sprintf(header, "Content-Length: %d\r\n\r\n", size);
    send(client_fd, header, strlen(header), 0);
	
	while((sendbytes = fread(body, 1, MAX_BUF_SIZE, fp)) > 0){
		send(client_fd, body, sendbytes, 0);
	}
	fclose(fp);
}

/* 浏览目录 */
void exec_dir(int client_fd, char *dirname, http_header *hr){
	
	char buf[MAX_BUF_SIZE], header[MAX_BUF_SIZE],img[MAX_BUF_SIZE], filename[MAX_BUF_SIZE];
	char web[50];
	DIR *dp;
	int num = 1;
	struct dirent *dirp;
	struct stat st;
	struct passwd *passwd;


	if(NULL == (dp = opendir(dirname))) err_sys("open dir fail", DEBUG);


	sprintf(buf, "<html><head><meta charset=""utf-8""><title>DIR List</title>");
	sprintf(buf, "%s<style type=""text/css"">img{ width:24px; height:24px;}</style></head>", buf);
	sprintf(buf, "%s<body bgcolor=""ffffff"" font-family=""Arial"" color=""#fff"" font-size=""14px"" >", buf);	
	sprintf(buf, "%s<h1>Index of %s</h1>", buf, dirname + 4);
	sprintf(buf, "%s<table cellpadding=\"0\" cellspacing=\"0\">", buf);
	if(hr->filename[strlen(hr->filename) - 1] != '/') strcat(hr->filename, "/");

	while(NULL != (dirp = readdir(dp))){
		if(0 == strcmp(dirp->d_name, ".") || 0 == strcmp(dirp->d_name, "..")) continue;
		sprintf(filename, "%s/%s", dirname, dirp->d_name);
		stat(filename, &st);
		passwd = getpwuid(st.st_uid);
	
		if(S_ISDIR(st.st_mode)) sprintf(img, "<img src='/icons/dir.png\'  />");
		else if(S_ISFIFO(st.st_mode)) sprintf(img, "<img src ='/icons/fifo.png'  />");
		else if(S_ISLNK(st.st_mode)) sprintf(img, "<img src ='/icons/link.png' />");
		else if(S_ISSOCK(st.st_mode)) sprintf(img, "<img src = '/icons/socket.png' />");
		else  sprintf(img, "<img src = '/icons/file.png'  />");

		sprintf(buf, "%s<tr  valign='middle'><td width='10'>%d</td><td width='30'>%s</td><td width='150'><a href='%s%s'>%s</a></td><td width='80'>%s</td><td width='100'>%d</td><td width='200'>%s</td></tr>", buf, num++, img, hr->filename, dirp->d_name, dirp->d_name, passwd->pw_name, (int)st.st_size, ctime(&st.st_atime));
		}
	
	closedir(dp);

    sprintf(header, "%s 200 OK\r\n", hr->version);  
	sprintf(header, "%sContent-length: %d\r\n", header, (int)strlen(buf));  
	sprintf(header, "%sContent-type: %s\r\n\r\n", header, "text/html"); 
	send(client_fd, header, strlen(header), 0);
	sprintf(buf, "%s</table></body></html>\r\n", buf);
	send(client_fd, buf, strlen(buf), 0);	
}	

void get_http_mime(char *ext, char *mime){
	
	if(0 == strcmp(ext, "html")){
		strcpy(mime, "text/html");
	}else if(0 == strcmp(ext, "gif")){
		strcpy(mime, "image/gif");
	}else if(0 == strcmp(ext, "jpg")){
		strcpy(mime, "image/jpeg");
	}else if(0 == strcmp(ext, "png")){
		strcpy(mime, "image/png");
	}else if(0 == strcmp(ext, "pdf")){
		strcpy(mime, "application/pdf"); 
	}else{
		strcpy(mime, "text/plain");
	}

}


/* 与 php-fpm 通信 */
void exec_php(int client_fd, http_header *hr){
	
	int fcgi_fd;
	
	/* 连接fastcgi服务器 */
	if(-1 == (fcgi_fd = conn_fastcgi())) return ;
	
	/* 发送数据 */
	if(-1 == (send_fastcgi(fcgi_fd, client_fd, hr))) return ;

	/* 接收数据 */
	recv_fastcgi(fcgi_fd, client_fd, hr);
	close(fcgi_fd);
}


/*
 * 连接php-fpm服务器
 * 成功返回socket描述符, 失败返回-1
 *
 * */
int conn_fastcgi(){	
	int fcgi_fd;
	struct sockaddr_in fcgi_sock;

	memset(&fcgi_sock, 0, sizeof(fcgi_sock));
	fcgi_sock.sin_family = AF_INET;
	fcgi_sock.sin_addr.s_addr = inet_addr(FCGI_HOST);
	fcgi_sock.sin_port = htons(FCGI_PORT);
	
	if(-1 == (fcgi_fd = socket(PF_INET, SOCK_STREAM, 0))){
		err_sys("fcgi socket()", DEBUG);
		return -1;
	}
	
	if(-1 == connect(fcgi_fd, (struct sockaddr *)&fcgi_sock, sizeof(fcgi_sock))){
		err_sys("php-fpm connect", DEBUG);
		return -1;
	}
	return fcgi_fd;
}

/* 
 * 与php-fpm通信
 * 成功返回1, 失败返回-1
 * 
 * */
int send_fastcgi(int fcgi_fd, int client_fd, http_header *hr){
		
	char filename[50];
	int requestId = client_fd, recvbytes, sendbytes;

	/* 发送FCGI_BEGIN_REQUEST */	
	FCGI_BeginRequestRecord beginRecord;
	beginRecord.body = makeBeginRequestBody(FCGI_RESPONDER);
	beginRecord.header =  makeHeader(FCGI_BEGIN_REQUEST, requestId, sizeof(beginRecord.body), 0);
	if(-1 == (sendbytes = send(fcgi_fd, &beginRecord, sizeof(beginRecord), 0))){
		err_sys("fcgi send beginRecord error", DEBUG);
		return -1;
	}


	getcwd(filename, sizeof(filename));
	strcat(filename, "/");
	strcat(filename, hr->path);
	buffer_path_simplify(filename, filename);
	char *params[][2] = {{"SCRIPT_FILENAME", filename}, {"REQUEST_METHOD", hr->method}, {"QUERY_STRING", hr->param}, {"CONTENT_TYPE", hr->contype },{"CONTENT_LENGTH", hr->conlength },{"", ""}};
	/* 发送 FCGI_PARAMS */
	int i, conLength, paddingLength;
	FCGI_ParamsRecord *paramsRecord;
	FCGI_Header emptyData;
	for(i = 0; params[i][0] != ""; i++){   // debug printf("%s : %s\n", params[i][0], params[i][1]);
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
			err_sys("fcgi send paramsRecord error", DEBUG);
			return -1;
		}
		free(paramsRecord);
	}
	/* 空的FCGI_PARAMS参数 */
	emptyData = makeHeader(FCGI_PARAMS, requestId, 0, 0);
	if(-1 == (sendbytes = send(fcgi_fd, &emptyData, FCGI_HEADER_LEN, 0))){
		err_sys("fcgi send paramsRecord empty error", DEBUG);
		return -1;
	}

	/*POST　并且有请求体 才会发送FCGI_STDIN数据 */
	char buf[8] = {0};
	int len = atoi(hr->conlength);
	int send_len;
	if(!strcmp("POST", hr->method)){
		// debug printf("post数据总长度:%d\n", len);
		// debug printf("%s\n", hr->content);
		while(len > 0){
			send_len = len > FCGI_MAX_LEN  ? FCGI_MAX_LEN : len;
			len -= send_len;
			FCGI_Header stdinHeader;
			paddingLength = (send_len %  8) == 0 ? 0 :  8 - (send_len % 8);
			stdinHeader = makeHeader(FCGI_STDIN, requestId, send_len, paddingLength);
			if(-1 == (sendbytes = send(fcgi_fd, &stdinHeader,FCGI_HEADER_LEN, 0))){
				err_sys("fcgi send stdinHeader error", DEBUG);
				return -1;
			}
			if(-1 == (sendbytes = send(fcgi_fd, hr->content, send_len, 0))){
				err_sys("fcgi send stdin", DEBUG);
				return -1;
			}
			// debug printf("单次发送大小:%d\n发送内容:\n%s\n", sendbytes, hr->content);
			if((paddingLength > 0) && (-1 == (sendbytes = send(fcgi_fd, buf, paddingLength, 0)))){
				err_sys("fcgi send stdin padding", DEBUG);
				return -1;
			}
			hr->content += send_len;
			// debug printf("长度%d\n", (int)strlen(hr->content));
		}
	}

	//debug  printf("%d\n", sendbytes);
	/* 发送空的FCGI_STDIN数据 */
	FCGI_Header emptyStdin = makeHeader(FCGI_STDIN, requestId, 0, 0);
	if(-1 == (sendbytes = send(fcgi_fd, &emptyStdin, FCGI_HEADER_LEN, 0))){
		err_sys("fcgi send stdinHeader enpty", DEBUG);
		return -1;
	}
	return 1;
}

void recv_fastcgi(int fcgi_fd, int client_fd, http_header *hr){

	FCGI_Header responseHeader;
	FCGI_EndRequestBody responseEnder;
	int recvbytes, recvId;
	char buf[8];
	char header[MAX_BUF_SIZE];
	int contentLength;
	int errlen = 0, outlen = 0;
	char *ok = NULL , *err = NULL;

	/*接受数据 */
	while(recv(fcgi_fd, &responseHeader, FCGI_HEADER_LEN, 0) > 0){
		recvId = (int)(responseHeader.requestIdB1 << 8) + (int)(responseHeader.requestIdB0);
		if(FCGI_STDOUT == responseHeader.type && recvId == client_fd){
			contentLength = ((int)responseHeader.contentLengthB1 << 8) + (int)responseHeader.contentLengthB0;
			outlen += contentLength;
			if(ok != NULL){
				ok = realloc(ok, outlen);
			}else{
				ok = (char *)malloc(contentLength);
			}
			recvbytes = recv(fcgi_fd, ok, contentLength, 0);
			if(-1 == recvbytes || contentLength != recvbytes) {
				err_sys("recv fcgi stdout fail", DEBUG); 
				return ;
			}
			if(responseHeader.paddingLength > 0){
				recvbytes = recv(fcgi_fd, buf, responseHeader.paddingLength, 0);
				if(-1 == recvbytes || recvbytes != responseHeader.paddingLength){
					err_sys("fcgi stdout padding fail", DEBUG);
					return;
				}
			}
		}else if(FCGI_STDERR == responseHeader.type && recvId == client_fd){
			contentLength = ((int)responseHeader.contentLengthB1 << 8) + (int)responseHeader.contentLengthB0;
			errlen += contentLength;
			if(err != NULL){
				err = realloc(err, errlen);
			}else{
				err = (char *)malloc(contentLength);
			}

			recvbytes = recv(fcgi_fd, err, contentLength, 0);
			if(-1 == recvbytes || contentLength != recvbytes){
				err_sys("recv fcgi stderr", DEBUG);
				return ;
			}

			if(responseHeader.paddingLength > 0){
				recvbytes = recv(fcgi_fd, buf, responseHeader.paddingLength, 0);
				if(-1 == recvbytes || recvbytes != responseHeader.paddingLength){
					err_sys("fcgi stdout padding", DEBUG);
					return ;
				}
			}
		}else if(FCGI_END_REQUEST == responseHeader.type && recvId == client_fd){
			recvbytes = recv(fcgi_fd, &responseEnder, sizeof(responseEnder), 0);
			
			if(-1 == recvbytes || sizeof(responseEnder) != recvbytes){
				free(err);
				free(ok);
				err_sys("fcgi recv end fail", DEBUG);
				return ;
			}

			/* 传输结束后构造http请求响应客户端 */
			send_client(ok, outlen, err, errlen, client_fd, hr);
			free(err);
			free(ok);
		}
	}

}

void send_client(char *ok, int outlen, char *err, int errlen, int client_fd, http_header *hr){
			
	char header[MAX_BUF_SIZE], header_buf[256];
	char *body, *start, *end, mime[256];
	int header_len, n;

	/* 请求头 */
	/*
	 * header
	 * \r\n\r\n
	 * body
	 */
	sprintf(header, "%s 200 OK\r\n", hr->version);  	
	body = strstr(ok, "\r\n\r\n") + 4;

	header_len = (int)(body - ok);	//头长度
	strncpy(header_buf, ok, header_len);
	sprintf(header, "%sContent-Length: %d\r\n", header,errlen + outlen - header_len);
			
	/*提取mime */
	start = strstr(header_buf, "Content-type");
	if(start != NULL){
		start = index(start, ':') + 2;
		end = index(start, '\r');
		n = end - start;
		strncpy(mime, start, n);
	}else{
		strcpy(mime, "text/html");
	}
	sprintf(header, "%sContent-Type: %s\r\n\r\n", header,mime);
	send(client_fd, header, strlen(header), 0);
	send(client_fd, body, outlen - header_len, 0);
	// debug printf("收到的stdin:\n%s\n收到的stderr:\n%s\n", ok, err);
	if(errlen > 0){
		send(client_fd, err, errlen, 0);
	}
}
