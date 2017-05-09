#include "fastcgi.h"
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

void parse_request(const int, char *, http_header *);
int get_line(const int, char *, int);
void *handle_request(void *);
void send_http_responce(int, const int, const char *, const http_header *);
void get_http_mime(char *, char *);
void exec_static(int, http_header *, int);
void exec_php(int, http_header *);
void exec_dir(int, char *, http_header *);
void err_exit(const char *);
void err_msg(const char *);

int main(int argc, char *argv[]){
	
	if(argc != 2){
		err_msg("Usage: ./studyHttpd <ip address>\n");
	}

	struct sockaddr_in server_sock, client_sock;
	int sockfd, client_fd;
	int sin_size;
	pthread_t ntid;
	
	if(-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))) err_exit("socket");
	printf("Socket id = %d\n", sockfd);

	server_sock.sin_family = AF_INET;
	server_sock.sin_port = htons(PORT);
	server_sock.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_sock.sin_zero), 8);

	int i = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	
	if(-1 == bind(sockfd, (struct sockaddr *)&server_sock, sizeof(struct sockaddr))) err_exit("bind");
	printf("Bind success\n");

	if(-1 == listen(sockfd, MAX_QUE_CONN_NM)) err_exit("listen");
	printf("Listening port = %d\n", PORT);

	sin_size = sizeof(client_sock);
	while(1){
	
		if(-1 == (client_fd = accept(sockfd, (struct sockaddr *) &client_sock, &sin_size))) err_exit("accept");
		
		if(pthread_create(&ntid, NULL, (void *)handle_request, &client_fd) != 0) err_exit("pthread_create");
	}
	close(sockfd);
    return 0;
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


	parse_request(client_fd, buf, &hr);
	
	/* 只支持 GET 和 POST请求 */
	if(strcasecmp(hr.method, "GET") && strcasecmp(hr.method, "POST")){
		//501未实现
		send_http_responce(client_fd, 501, "Not Implemented", &hr);
		close(client_fd);
		return NULL;
	}
	/* 简单判断HTTP协议 */
	if(NULL == strstr(hr.version, "HTTP/")){
		// 505协议版本不支持
		send_http_responce(client_fd, 505, "HTTP Version Not Supported", &hr);
		close(client_fd);
		return NULL;
	}
	
	/* 访问文件 */
	if(-1 == stat(hr.path, &st)){
		/* 文件未找到 */
		while((recvbytes > 0) && strcmp("\n", buf)) recvbytes = get_line(client_fd, buf, MAX_BUF_SIZE);
		send_http_responce(client_fd, 404, "Not Found", &hr);
	}else{
		if((st.st_mode & S_IFMT) == S_IFDIR){
			exec_dir(client_fd, hr.path, &hr);
		}else{
		
			/* 提取文件类型 */
			char path[256];
			strcpy(path, hr.path);
			strtok(path, ".");
			strcpy(hr.ext, strtok(NULL, "."));
			if(0 == strcmp(hr.ext, "php")){	//php 文件
				if(0 != access(hr.path, X_OK)) {
					send_http_responce(client_fd, 403, "Forbidden", &hr);
					close(client_fd);
					return ;
				}
				exec_php(client_fd, &hr);
			}else{	//静态文件	
				if(!S_ISREG(st.st_mode) && !(S_IRUSR & st.st_mode)) {
					send_http_responce(client_fd, 403, "Forbidden", &hr);
					close(client_fd);
					return ;
				}
				exec_static(client_fd, &hr, st.st_size);
			}
		}
	}

	//shutdown(client_fd, SHUT_RD);
	//close(client_fd);	
}


/* 解析HTTP请求信息 */
void parse_request(const int client_fd, char *buf, http_header *hr){
	int recvbytes;
	int i = 0, j = 0;
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
	sprintf(hr->path, "htdocs%s", hr->filename);
}

/* 获取http请求一行数据 */
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

void err_exit(const char *msg){
	perror(msg);
	exit(1);
}

void err_msg(const char *msg){
	fprintf(stderr, "%s", msg);
	exit(1);
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

/* 列举目录 */
void exec_dir(int client_fd, char *dirname, http_header *hr){

	char buf[MAX_BUF_SIZE], header[MAX_BUF_SIZE],img[MAX_BUF_SIZE], filename[MAX_BUF_SIZE];
	DIR *dp;
	int num = 1;
	struct dirent *dirp;
	struct stat st;
	struct passwd *passwd;
	
	if(NULL == (dp = opendir(dirname))) err_exit("open dir");


	sprintf(buf, "<html><head><meta charset=""utf-8""><title>DIR List</title>");
	sprintf(buf, "%s<style type=""text/css"">img{ width:24px; height:24px;}</style></head>", buf);
	sprintf(buf, "%s<body bgcolor=""ffffff"" font-family=""Arial"" color=""#fff"" font-size=""14px"" >", buf);	
	sprintf(buf, "%s<h1>Index of /%s</h1>", buf, dirname);
	sprintf(buf, "%s<table cellpadding=\"0\" cellspacing=\"0\">", buf);
	if(hr->filename[strlen(hr->filename) - 1] != '/') strcat(hr->filename, "/");

	while(NULL != (dirp = readdir(dp))){
		if(0 == strcmp(dirp->d_name, ".") || 0 == strcmp(dirp->d_name, "..")) continue;
		sprintf(filename, "%s/%s", dirname, dirp->d_name);
		stat(filename, &st);
		passwd = getpwuid(st.st_uid);
	
		/* 这里img　变量有点问题.   它会的当前路径是http请求的路径, 而不是服务器的路径 */
		if(S_ISDIR(st.st_mode)) sprintf(img, "<img src='./img/dir.png\'  />");
		else if(S_ISFIFO(st.st_mode)) sprintf(img, "<img src ='./img/fifo.png'  />");
		else if(S_ISLNK(st.st_mode)) sprintf(img, "<img src ='./img/link.png' />");
		else if(S_ISSOCK(st.st_mode)) sprintf(img, "<img src = './img/socket.png' />");
		else  sprintf(img, "<img src = './img/file.png'  />");

		sprintf(buf, "%s<tr  valign='middle'><td width='10'>%d</td><td width='30'>%s</td><td width='150'><a href='%s%s'>%s</a></td><td width='80'>%s</td><td width='100'>%d</td><td width='200'>%s</td></tr>", buf, num++, img, hr->filename, dirp->d_name, dirp->d_name, passwd->pw_name, (int)st.st_size, ctime(&st.st_atime));
		}
	
	closedir(dp);
	sprintf(buf, "%s</table></body></html>", buf);

	/* 发送响应 */
    sprintf(header, "%s 200 OK\r\n", hr->version);  
	sprintf(header, "%sContent-length: %d\r\n", header, (int)strlen(buf));  
	sprintf(header, "%sContent-type: %s\r\n\r\n", header, "text/html"); 
	send(client_fd, header, strlen(header), 0);
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
	}else{
		strcpy(mime, "text/plain");
	}

}

void exec_php(int client_fd, http_header *hr){
	
	int sendbytes, recvbytes;
	int fcgi_fd;
	int requestId;
	struct sockaddr_in fcgi_sock;
	char filename[50];
	char header[MAX_BUF_SIZE];

	memset(&fcgi_sock, 0, sizeof(fcgi_sock));
	fcgi_sock.sin_family = AF_INET;
	fcgi_sock.sin_addr.s_addr = inet_addr(FCGI_HOST);
	fcgi_sock.sin_port = htons(FCGI_PORT);
	
	if(-1 == (fcgi_fd = socket(PF_INET, SOCK_STREAM, 0)))  err_exit("fcgi socket");

	requestId = fcgi_fd;

	if(-1 == connect(fcgi_fd, (struct sockaddr *)&fcgi_sock, sizeof(fcgi_sock))) err_exit("fcgi connect");

	FCGI_BeginRequestRecord beginRecord;
	beginRecord.body = makeBeginRequestBody(FCGI_RESPONDER);
	beginRecord.header =  makeHeader(FCGI_BEGIN_REQUEST, requestId, sizeof(beginRecord.body), 0);
	
	if(-1 == (sendbytes = send(fcgi_fd, &beginRecord, sizeof(beginRecord), 0))) err_exit("fcgi send beginRecord");
	

	getcwd(filename, sizeof(filename));
	strcat(filename, "/");
	strcat(filename, hr->path);
	  
	char *params[][2] = {
		{"SCRIPT_FILENAME", filename}, 
		{"REQUEST_METHOD", hr->method}, 
		{"QUERY_STRING", hr->param}, 
		{"", ""}
	};

	
	int i, conLength, paddingLength;
	FCGI_ParamsRecord *paramsRecord;
	for(i = 0; params[i][0] != ""; i++){
		conLength = strlen(params[i][0]) + strlen(params[i][1]) + 2;
		paddingLength = (conLength % 8) == 0 ? 0 : 8 - (conLength % 8);
		paramsRecord = (FCGI_ParamsRecord *)malloc(sizeof(FCGI_ParamsRecord) + conLength + paddingLength);
		paramsRecord->nameLength = (unsigned char)strlen(params[i][0]);    // 填充参数值
		paramsRecord->valueLength = (unsigned char)strlen(params[i][1]);   // 填充参数名
		paramsRecord->header = makeHeader(FCGI_PARAMS, requestId, conLength, paddingLength);
		memset(paramsRecord->data, 0, conLength + paddingLength);
		memcpy(paramsRecord->data, params[i][0], strlen(params[i][0]));
		memcpy(paramsRecord->data + strlen(params[i][0]), params[i][1], strlen(params[i][1]));
		if(-1 == (sendbytes = send(fcgi_fd, paramsRecord, 8 + conLength + paddingLength, 0))) err_exit("fcgi send paramsRecord");
	}


	FCGI_Header stdinHeader;
	stdinHeader = makeHeader(FCGI_STDIN, requestId, 0, 0);
	if(-1 == (sendbytes = send(fcgi_fd, &stdinHeader, sizeof(stdinHeader), 0))) err_exit("fcgi send stdinHeader");
	

	FCGI_Header responseHeader;
	int contentLength;
	char *msg;
	if(-1 == recv(fcgi_fd, &responseHeader, sizeof(responseHeader), 0)) err_exit("fcgi recv");
	if(FCGI_STDOUT == responseHeader.type){
		contentLength = ((int)responseHeader.contentLengthB1 << 8) + (int)responseHeader.contentLengthB0;
		msg = (char *)malloc(contentLength);
		recv(fcgi_fd, msg, contentLength, 0);
	}


	/* 发送响应 */
    sprintf(header, "%s 200 OK\r\n", hr->version);  	
	sprintf(header, "%sContent-Length: %d\r\n", header, contentLength);
	send(client_fd, header, strlen(header), 0);
	send(client_fd, msg, contentLength, 0);

	printf("%s%s\n", header, msg);
	free(msg);
	close(fcgi_fd);
}




