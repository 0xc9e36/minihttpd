#ifndef _COMMON_H_
#define _COMMON_H_

#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<error.h>
#include<stdlib.h>
#define	ROOT	"../"
#define WEB		"./htdocs"
#define DEBUG __FILE__, __LINE__, __FUNCTION__ 


void err_sys(const char *msg, const char *filename, int line, const char *func);
void err_user(char *msg);
int buffer_path_simplify(char *dest, char *src); 

#endif
