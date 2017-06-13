#include "http.h"
#include "rio.h"

/* 请求头信息 */
header_handle headers[] = {
	{"Host", header_ignore},
	{"Connection", header_connection},
	{"If-Modified-Since", header_modified},
	{"Content-Type", header_contype},
	{"", header_ignore},
};

int init_http_request(http_request *request, int sockfd, int epfd, config_t *config){
	request->sockfd = sockfd;
	request->state = 0;
	request->pos = 0;
	request->conlength = request->read_length = 0;
	request->last = 0;
	request->epfd = epfd;
	request->alalyzed = 0;
	//初始化请求头链表
    INIT_LIST_HEAD(&(request->list)); 
	return 1;
}

/*
 * 初始化socket
 * 成功返回socket描述符, 失败返回-1
 */
int init_server(int port){
	int sockfd;
	struct sockaddr_in server_sock;
	if(-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))){
		err_sys("socket() fail", DEBUGPARAMS);
		return -1;
	}
	printf("------启动server------\n");
	printf("Socket id = %d\n", sockfd);

	server_sock.sin_family = AF_INET;
	server_sock.sin_port = htons(port);
	server_sock.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_sock.sin_zero), 8);

	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	
	if(-1 == bind(sockfd, (struct sockaddr *)&server_sock, sizeof(struct sockaddr))){
		close(sockfd);
		err_sys("bind() fail", DEBUGPARAMS);
		return -1;
	}
	printf("Bind success\n");

	if(-1 == listen(sockfd, MAX_QUE_CONN_NM)){
		close(sockfd);
		err_sys("listen() fail", DEBUGPARAMS);
		return -1;
	}
	printf("Listening port = %d\n", PORT);

	if(-1 == set_non_blocking(sockfd)){
		close(sockfd);
		return -1;
	}
	return sockfd;
}

/* 设置非阻塞IO */
int set_non_blocking(int sockfd){
	int flag;
	if((flag = fcntl(sockfd, F_GETFL, 0)) < 0){
		err_sys("get open Identification fail", DEBUGPARAMS);
		return -1;
	}
	
	flag |= O_NONBLOCK;
	if(fcntl(sockfd, F_SETFL, flag) < 0){	
		err_sys("set non-blocking IO fail", DEBUGPARAMS);
		return -1;
	}
	return 1;
}

/* 处理http请求 */
void *handle_request(void *arg){

	http_request *hr = (http_request *)arg;

	char *plast = NULL, path[HTTP_BUF_SIZE], param[HTTP_BUF_SIZE];
	struct stat st;
	size_t remain_size;
	int n, res;

	while(1){
		//空闲地址
		plast = &hr->buf[hr->last % HTTP_BUF_SIZE];
		//空闲大小
		remain_size = min(HTTP_BUF_SIZE - (hr->last - hr->pos) - 1, HTTP_BUF_SIZE - hr->last % HTTP_BUF_SIZE);  
	
		if (0 == (n = read(hr->sockfd, plast, remain_size))){
			//浏览器关闭时， 会在下一个epoll_wait()返回时，从fd上面读取0
			//printf("无数据可读\n");
			//err_sys("client close", DEBUGPARAMS);
			http_close(hr);
			return NULL;
		}else if(n < 0){
			if((errno == EAGAIN) || (errno == EWOULDBLOCK)) break;	
			err_sys("read error", DEBUGPARAMS);
			http_close(hr);
			return NULL;
		}


		hr->last += n;
	
		/* 解析请求行 */
		res = http_parse_line(hr);
		
		if(res == HTTP_AGAIN){
			continue;
		}else if(res != HTTP_OK){
			err_sys("parse request line error", DEBUGPARAMS);
			http_close(hr);
			return NULL;
		}

		/* 解析请求头 */
		res = http_parse_head(hr);

		if(res == HTTP_AGAIN){
			continue;
		}else if(res != HTTP_OK){
			err_sys("parse request header error", DEBUGPARAMS);
			http_close(hr);
			return NULL;
		}
	
		  
						   
		/* 解析请求主体 */
		res = http_parse_body(hr);

		if(res == HTTP_AGAIN){
			continue;
		}else if(res != HTTP_OK){
			err_sys("parst request body error", DEBUGPARAMS);
			http_close(hr);
			return NULL;
		}

		http_response *response = (http_response *)malloc(sizeof(http_response));
		init_response(response, hr->sockfd);

		/* 解析请求数据 */
		parse_request(hr);

		/* 只支持 GET 和 POST请求 */
		if(hr->method == UNKNOW){
			//501未实现
			send_http_responce(501, "NOT IMPLEMENTED", hr);
			continue;
		}
	
		/* 简单判断HTTP协议, http1.0 和 http1.1  */
	  	if(strcasecmp(hr->version, "HTTP/1.0") && strcasecmp(hr->version, "HTTP/1.1")){
			// 505协议版本不支持
			send_http_responce(505, "HTTP Version Not Supported", hr);
			continue;
		}

		/* 访问文件 */
		if(-1 == stat(hr->path, &st)){
			/* 404文件未找到 */
			send_http_responce(404, "Not Found", hr);
			continue;
		}
	
		//最后修改时间
		response->ctime = st.st_mtime;

		paser_header(hr, response);

		//目录浏览
		if((st.st_mode & S_IFMT) == S_IFDIR){
			exec_dir(hr, response);
		}else{	

			if(0 == strcmp(hr->ext, "php")){	//php 文件
				if(!S_ISREG(st.st_mode) || !(S_IXUSR & st.st_mode)) {
					// 403 无执行权限
					send_http_responce(403, "Forbidden", hr);
				}else{
					//执行php  -> 调用php-fpm
					//printf("读取内容%s\n", hr->content);
					exec_php(hr, response);
				}
			}else{	//静态文件	
				if(!S_ISREG(st.st_mode) || !(S_IRUSR & st.st_mode)) {
					// 403 无读权限
					send_http_responce(403, "Forbidden", hr);
				}else{
					exec_static(hr, response, st.st_size);
				}
			}
		}
		
		/* 关闭连接 */
		if(!response->keep_alive){

			free(response);
			http_close(hr);
			return NULL;
		}
	
		free(response);
	}

	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	event.data.ptr = hr;
	
	epoll_ctl(hr->epfd,EPOLL_CTL_MOD, hr->sockfd, &event);
	return NULL;
}

void init_response(http_response *rs, int sockfd){
	rs->sockfd = sockfd;
	rs->keep_alive = 0;
	rs->modified = 1;
	rs->status = 0;
}

void http_close(http_request *hr){
	//printf("关闭连接\n");
	close(hr->sockfd);
	free(hr);
}

/* 解析http请求头 */
void paser_header(http_request *hr, http_response *response){
	list_head *pos;
	http_header *hd;
	header_handle *header_h;
	int len;

	list_for_each(pos, &(hr->list)){
		hd = list_entry(pos, http_header, list);
		for(header_h = headers; strlen(header_h->key) > 0; header_h++){
				
			if(strncmp(hd->key_start, header_h->key, hd->key_end - hd->key_start) == 0){
				len = hd->val_end - hd->val_start;
				(*(header_h->handle))(hr, response, hd->val_start, len);
				break;
			}
		}
	
		list_del(pos);
		free(hd);
	}

		//printf("----------------\n");
}

/* 解析HTTP请求信息 */
int parse_request(http_request *hr){

	char web[50];
	/* 根目录 */
	sprintf(web, "%s%s", ROOT, WEB);
	//printf("%s\n", hr->root);

	memset(hr->ext, '\0', sizeof(char)*10);
	memset(hr->version, '\0', sizeof(char)*10);
	memset(hr->filename, '\0', sizeof(char)*1024);
	memset(hr->param, '\0', sizeof(char)*1024);
	memset(hr->path, '\0', sizeof(char)*1024);

	
	//strncpy(hr->url, hr->url_start, hr->url_end - hr->url_start);

	/*请求参数 */
	if(strstr(hr->url, "?")){
		strcpy(hr->filename, strtok(hr->url, "?"));
		strcpy(hr->param, strtok(NULL, "?"));
	}else{
		strcpy(hr->filename, hr->url);
		strcpy(hr->param, "");
	}

	/* 版本信息 */
	sprintf(hr->version, "HTTP/%d.%d", hr->http_major, hr->http_minor);

	/* 文件路径 */
	sprintf(hr->path, "%s%s", web, hr->filename);

	/* 扩展名 */
	char *t_path;
	t_path = strrchr(hr->path, '.');
	strcpy(hr->ext, (t_path + 1));

	//printf("请求url%s\n请求路径%s\n请求参数%s\n版本%s\n\n\n", hr->url, hr->path, hr->param, hr->version);

	return 1;
}

/* 发送http响应信息 */
void send_http_responce(const int http_code, const char *msg, const http_request * hr){
	char header[MAX_BUF_SIZE], body[MAX_BUF_SIZE];
	sprintf(body, "<html><body>%d:%s<hr /><em>studyHttpd Web Service</em></body></html>", http_code, msg);

	sprintf(header, "HTTP/%d.%d %d %s\r\n",hr->http_major, hr->http_minor, http_code, msg);
	sprintf(header, "%sContent-Type: text/html\r\n", header);
	sprintf(header, "%sConnection: close\r\n", header);
	sprintf(header, "%sContent-Length: %d\r\n\r\n", header, (int)strlen(body));
	
	// debug printf("\n%s%s\n", header, body);

	rio_writen(hr->sockfd, header, strlen(header));
	rio_writen(hr->sockfd, body, strlen(body));
	
}

/* 处理静态文件 */
void exec_static(http_request *hr,  http_response *response, int size){
	FILE *fp;
	int sendbytes;
	char header[MAX_BUF_SIZE];
	char body[MAX_BUF_SIZE];
	char mime[MAX_BUF_SIZE];
	char time_str[100];
	struct tm tm;
	
	memset(time_str, '\0', sizeof(char) * 100);

	fp = fopen(hr->path, "rb");
	
	if(NULL == fp){
		send_http_responce(404, "Not Found", hr);
		return ;
	}
	get_http_mime(hr->ext, mime);

	if(response->modified){

	    sprintf(header, "%s 200 OK\r\n", hr->version);  
		sprintf(header, "%sContent-length: %d\r\n", header, size); 
	    localtime_r(&(response->ctime), &tm);
		strftime(time_str, 100,  "%a, %d %b %Y %H:%M:%S GMT", &tm);
		sprintf(header, "%sLast-Modified: %s\r\n", header, time_str);
		sprintf(header, "%sContent-type: %s\r\n", header, mime); 

	}else{

		sprintf(header, "%s 304 Not Modified\r\n", hr->version);  
	}

	if(response->keep_alive){
		sprintf(header, "%sConnection: keep-alive\r\n", header);
		sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, TIMEOUT);
	}

	sprintf(header, "%s\r\n", header);

    rio_writen(hr->sockfd, header, strlen(header));
	

	hr->url_start = hr->url_end = NULL;

	while((sendbytes = fread(body, 1, MAX_BUF_SIZE, fp)) > 0){

		rio_writen(hr->sockfd, body, sendbytes);
	}
	fclose(fp);
}

/* 浏览目录 */
void exec_dir(http_request *hr, http_response *response){

	char buf[MAX_BUF_SIZE], header[MAX_BUF_SIZE],img[MAX_BUF_SIZE], filename[MAX_BUF_SIZE];
	DIR *dp;
	int num = 1;
	struct dirent *dirp;
	struct stat st;
	struct passwd *passwd;

	if(NULL == (dp = opendir(hr->path))) err_sys("open dir fail", DEBUGPARAMS);

	sprintf(buf, "<html><head><meta charset=""utf-8""><title>DIR List</title>");
	sprintf(buf, "%s<style type=""text/css"">img{ width:24px; height:24px;}</style></head>", buf);
	sprintf(buf, "%s<body bgcolor=""ffffff"" font-family=""Arial"" color=""#fff"" font-size=""14px"" >", buf);	
	sprintf(buf, "%s<h1>Index of %s</h1>", buf, hr->path + 4);
	sprintf(buf, "%s<table cellpadding=\"0\" cellspacing=\"0\">", buf);
	if(hr->filename[strlen(hr->filename) - 1] != '/') strcat(hr->filename, "/");


	while(NULL != (dirp = readdir(dp))){
		if(0 == strcmp(dirp->d_name, ".") || 0 == strcmp(dirp->d_name, "..")) continue;
		sprintf(filename, "%s/%s", hr->path, dirp->d_name);
		stat(filename, &st);
		passwd = getpwuid(st.st_uid);
	
		if(S_ISDIR(st.st_mode)) sprintf(img, "<img src='/icons/dir.png\'  />");
		else if(S_ISFIFO(st.st_mode)) sprintf(img, "<img src ='/icons/fifo.png'  />");
		else if(S_ISLNK(st.st_mode)) sprintf(img, "<img src ='/icons/link.png' />");
		else if(S_ISSOCK(st.st_mode)) sprintf(img, "<img src = '/icons/socket.png' />");
		else  sprintf(img, "<img src = '/icons/file.png'  />");

		sprintf(buf, "%s<tr  valign='middle'><td width='10'>%d</td><td width='30'>%s</td><td width='150'><a href='%s%s'>%s</a></td><td width='80'>%s</td><td width='100'>%d</td><td width='200'>%s</td></tr>", buf, num++, img, hr->filename, dirp->d_name, dirp->d_name, passwd->pw_name, (int)st.st_size, ctime(&st.st_atime));
	}
	
	sprintf(buf, "%s</table></body></html>", buf);

	closedir(dp);

	
	struct tm tm;
	char time_str[100];
	memset(time_str, '\0', sizeof(char) * 100);
	if(response->modified){
	    sprintf(header, "%s 200 OK\r\n", hr->version);  
		sprintf(header, "%sContent-length: %d\r\n", header, (int)strlen(buf)); 
	    localtime_r(&(response->ctime), &tm);
		strftime(time_str, 100,  "%a, %d %b %Y %H:%M:%S GMT", &tm);
		sprintf(header, "%sLast-Modified: %s\r\n", header, time_str);
		sprintf(header, "%sContent-type: %s\r\n", header, "text/html"); 

	}else{

		sprintf(header, "%s 304 Not Modified\r\n", hr->version);  
	}

	if(response->keep_alive){
		sprintf(header, "%sConnection: keep-alive\r\n", header);
		sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, TIMEOUT);
	}

	sprintf(header, "%s\r\n", header);

	send(hr->sockfd, header, strlen(header), 0);
	
	if(!response->modified) return ;	
	
	send(hr->sockfd, buf, strlen(buf), 0);	
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


int header_ignore(http_request *hr, http_response *response, char *val, int len){
	(void) hr;
	(void) response;
	(void) val;
	(void) len;
	//printf("该请求头忽略\n");
}

int header_contype(http_request *hr, http_response *response, char *val, int len){
	
	(void)response;
	memset(hr->contype, '\0', 255);
	
	strncpy(hr->contype, val, len);

	//printf("%s\n", hr->contype);
}

int header_connection(http_request *hr, http_response *response, char *val, int len){
	
	
	if(strncasecmp(val, "keep-alive", len) == 0){
		response->keep_alive = 1;
	}

	hr->alalyzed = 0;
	return 1;
}

int header_modified(http_request *hr, http_response *response, char *val, int len){
	
	(void)hr;
	(void)len;

	struct tm time;
	//将字符串转化为struct tm结构失败
	if(strptime(val, "%a, %d %b %Y %H:%M:%S GMT", &time) == 0)  return 1;
	
	time_t c_time = mktime(&time);
	double df_time = difftime(response->ctime, c_time);
	/* 1微秒之内算作未改变 */
	if(fabs(df_time) < 1e-6){
		response->modified = 0;
		response->status = HTTP_NOT_MODIFIED; 
	}

	return 1;
}


void exec_php(http_request *hr, http_response *rs){
	
	int fcgi_fd;
	
	if(-1 == (fcgi_fd = conn_fastcgi())) return ;
	
	if(-1 == (send_fastcgi(fcgi_fd,hr))) return ;

	recv_fastcgi(fcgi_fd, hr, rs);
	
	close(fcgi_fd);
}


void send_client(char *ok, int outlen, char *err, int errlen,http_request *hr, http_response *rs){
			
	char header[MAX_BUF_SIZE], header_buf[256];
	char *body, *start, *end, mime[256];
	int header_len, n;
	char time_str[100];
	struct tm tm;

	memset(mime, '\0', 256);
	/* 请求头 */
	/*
	printf("发数据完毕\n");
	 * header
	 * \r\n\r\n
	 * body
	 */
	body = strstr(ok, "\r\n\r\n") + 4;

	header_len = (int)(body - ok);	//头长度
	strncpy(header_buf, ok, header_len);
			
	start = strstr(header_buf, "Content-type");
	if(start != NULL){
		start = index(start, ':') + 2;
		end = index(start, '\r');
		n = end - start;
		strncpy(mime, start, n);
	}else{
		strcpy(mime, "text/html");
	}

	if(rs->modified){
	    sprintf(header, "%s 200 OK\r\n", hr->version);  
		sprintf(header, "%sContent-Length: %d\r\n", header,errlen + outlen - header_len);
	    localtime_r(&(rs->ctime), &tm);
		strftime(time_str, 100,  "%a, %d %b %Y %H:%M:%S GMT", &tm);
		sprintf(header, "%sLast-Modified: %s\r\n", header, time_str);
		sprintf(header, "%sContent-type: %s\r\n", header, mime); 

	}else{

		sprintf(header, "%s 304 Not Modified\r\n", hr->version);  
	}

	if(rs->keep_alive){
		sprintf(header, "%sConnection: keep-alive\r\n", header);
		sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, TIMEOUT);
	}
	
	sprintf(header, "%s\r\n", header);
	send(hr->sockfd, header, strlen(header), 0);
	send(hr->sockfd, body, outlen - header_len, 0);
	// debug printf("收到的stdin:\n%s\n收到的stderr:\n%s\n", ok, err);
	if(errlen > 0){
		send(hr->sockfd, err, errlen, 0);
	}
}

