#ifndef _COMMON_H_
#define _COMMON_H_


/* 定义一些公用的内容，　比如错误处理... */

#include<stdio.h>
#include<signal.h>
#include<string.h>
#include<errno.h>
#include<error.h>
#include<stdlib.h>

#define	ROOT	"../"
#define WEB		"./htdocs"
#define DEBUGPARAMS __FILE__, __LINE__, __FUNCTION__ 


void err_sys(const char *msg, const char *filename, int line, const char *func);
void err_user(char *msg);
int buffer_path_simplify(char *dest, char *src); 
int init_signal();

#endif
