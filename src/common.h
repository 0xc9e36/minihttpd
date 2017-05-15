#ifndef _COMMON_H_
#define _COMMON_H_

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#define	ROOT	"../"
#define WEB		"./htdocs"

void err_sys(const char *);
void err_user(const char *);
void err_msg(const char *);
int buffer_path_simplify(char *dest, char *src); 

#endif
