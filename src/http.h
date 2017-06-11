#ifndef _HTTP_H_
#define _HTTP_H_

#include "http_info.h"
#include "http_parse.h"
#include "fastcgi.h"
#include "common.h"
#include "epoll.h"
#include "list.h"
#include<stdio.h>
#include<string.h>
#include<dirent.h>
#include<pwd.h>
#include<math.h>
#include<time.h>
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



int init_server(int);
void init_response(http_response *rs, int sockfd);
int set_non_blocking(int sockfd);
int init_http_request(http_request *, int, int, config_t *);
int parse_request(http_request *);
int get_line(const int, char *, int);
void *handle_request(void *);
void send_http_responce(const int, const char *, const http_request *);
void get_http_mime(char *, char *);
void exec_static(http_request *, http_response *, int);
void exec_php(http_request *, http_response *);
void exec_dir(http_request *, http_response *);
char *get_http_Val(const char* str, const char *substr);
void send_client(char *, int, char *, int, http_request *, http_response *);
void http_close(http_request *hr);

int header_ignore(http_request *, http_response *, char *, int);
int header_connection(http_request *, http_response *, char *, int);
int header_modified(http_request *, http_response *, char *, int);
int header_contype(http_request *, http_response *, char *, int);

void paser_header(http_request *hr, http_response *response);

#endif
