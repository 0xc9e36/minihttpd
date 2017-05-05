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
#define		MAX_QUE_CONN_NM	5
#define		MAX_BUF_SIZE	8192

/* http请求信息结构体, 只存储几个关键信息, 其他忽略 */
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

void err_exit(const char *);
void err_msg(const char *);
void parse_request(const int, char *, http_header *);
int get_line(const int, char *, int);
void *handle(void *);
void http_error(int, const int, const char *, const http_header *);
char *get_mime(char *, char *);
void execStatic(int, http_header *, int);
void execDir(int, char *, http_header *);

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

//	int i = 1;
//	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	
	if(-1 == bind(sockfd, (struct sockaddr *)&server_sock, sizeof(struct sockaddr))) err_exit("bind");
	printf("Bind success\n");

	if(-1 == listen(sockfd, MAX_QUE_CONN_NM)) err_exit("listen");
	printf("Listening port = %d\n", PORT);

	sin_size = sizeof(client_sock);
	while(1){
		if(-1 == (client_fd = accept(sockfd, (struct sockaddr *) &client_sock, &sin_size))) err_exit("accept");
		if(pthread_create(&ntid, NULL, (void *)handle, &client_fd) != 0) err_exit("pthread_create");
	}
	close(sockfd);
    return 0;
}

void *handle(void *arg){
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
		http_error(client_fd, 501, "Not Implemented", &hr);
		close(client_fd);
		return NULL;
	}
	/* 简单判断HTTP协议 */
	if(NULL == strstr(hr.version, "HTTP/")){
		// 505协议版本不支持
		http_error(client_fd, 505, "HTTP Version Not Supported", &hr);
		close(client_fd);
		return NULL;
	}
	
	/* 访问文件 */
	if(-1 == stat(hr.path, &st)){
		/* 文件未找到 */
		while((recvbytes > 0) && strcmp("\n", buf)) recvbytes = get_line(client_fd, buf, MAX_BUF_SIZE);
		http_error(client_fd, 404, "Not Found", &hr);
	}else{
		if((st.st_mode & S_IFMT) == S_IFDIR){
			execDir(client_fd, hr.path, &hr);
		}else{
		
			/* 提取文件类型 */
			char path[256];
			strcpy(path, hr.path);
			strtok(path, ".");
			strcpy(hr.ext, strtok(NULL, "."));
	
			if(0 == strcmp(hr.ext, "php")){	//php 文件
				printf("php文件\n");
			}else{	//静态文件	
				if(!S_ISREG(st.st_mode) || !(S_IRUSR & st.st_mode)) {
					http_error(client_fd, 403, "Forbidden", &hr);
					return ;
				}
				execStatic(client_fd, &hr, st.st_size);
				//http_error(client_fd, 415, "Unsupported Media Type", &hr);
			}
		}
	}

	close(client_fd);	
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

void http_error(int client_fd, const int http_code, const char *msg, const http_header * hr){
	char header[MAX_BUF_SIZE], body[MAX_BUF_SIZE];
	sprintf(body, "<html><title>Server Error</title>");
	sprintf(body, "%s<body>\r\n", body);
	sprintf(body, "%s%d: %s\r\n", body, http_code, msg);
	sprintf(body, "%s<hr/><em>studyHttpd Web Service</em>\r\n</body></html>", body);

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
void execStatic(int client_fd, http_header *hr, int size){
	FILE *fp;
	int sendbytes;
	char header[MAX_BUF_SIZE];
	char body[MAX_BUF_SIZE];
	char mime[MAX_BUF_SIZE];

	fp = fopen(hr->path, "rb");
	
	if(NULL == fp){
		http_error(client_fd, 404, "Not Found", hr);
		return ;
	}
	/* 获取文件MIME类型 text/html image/png ... */
	get_mime(hr->ext, mime);

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
void execDir(int client_fd, char *dirname, http_header *hr){

	char buf[MAX_BUF_SIZE], header[MAX_BUF_SIZE],img[MAX_BUF_SIZE], filename[MAX_BUF_SIZE];
	DIR *dp;
	int num = 1;
	struct dirent *dirp;
	struct stat st;
	struct passwd *passwd;
	
	if(NULL == (dp = opendir(dirname))) err_exit("open dir");


	sprintf(buf, "<html><head><meta charset=""utf-8""><title>DIR List</title>");
	sprintf(buf, "%s<style type=""text/css"">a:link{text-decoration:none}</style></head>", buf);
	sprintf(buf, "%s<body bgcolor=""ffffff"" font-family=""Arial"" color=""#fff"" font-size=""14px"" >", buf);	
	sprintf(buf, "%s<h1>Index of /%s</h1>", buf, dirname);
	if(hr->filename[strlen(hr->filename) - 1] != '/') strcat(hr->filename, "/");

	while(NULL != (dirp = readdir(dp))){
		if(0 == strcmp(dirp->d_name, ".") || 0 == strcmp(dirp->d_name, "..")) continue;
		sprintf(filename, "%s/%s", dirname, dirp->d_name);
		stat(filename, &st);
		passwd = getpwuid(st.st_uid);
		
		if(S_ISDIR(st.st_mode)) sprintf(img, "<img src=\"img/dir.png\" width=\"24px\" height=\"24px\" />");
		else if(S_ISFIFO(st.st_mode)) sprintf(img, "<img src =\"img/fifo.png\" width=\"24px\" height=\"24px\" />");
		else if(S_ISLNK(st.st_mode)) sprintf(img, "<img src =\"img/link.png\" width=\"24px\" height=\"24px\" />");
		else if(S_ISSOCK(st.st_mode)) sprintf(img, "<img src = \"img/socket.png\" width=\"24px\" height=\"24px\" />");
		else  sprintf(img, "<img src = \"img/file.png\" width=\"24px\" height=\"24px\" />");
	
		sprintf(buf,"%s<p><pre>%d %s""<a href=%s%s"">%s</a> \t\t %s \t\t %d \t\t%s</pre></p>",buf,num++,img,hr->filename,dirp->d_name,dirp->d_name, passwd->pw_name,(int)st.st_size,ctime(&st.st_atime));
	}
	
	closedir(dp);
	sprintf(buf, "%s</body></html>", buf);

	/* 发送 */
    sprintf(header, "%s 200 OK\r\n", hr->version);  
	sprintf(header, "%sContent-length: %d\r\n", header, (int)strlen(buf));  
	sprintf(header, "%sContent-type: %s\r\n\r\n", header, "text/html"); 
	send(client_fd, header, strlen(header), 0);
	send(client_fd, buf, strlen(buf), 0);

}	

char *get_mime(char *ext, char *mime){
	
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
