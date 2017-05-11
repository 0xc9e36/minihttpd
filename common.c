#include "common.h"


void err_sys(const char *msg){
	perror(msg);
	exit(1);
}

void err_user(const char *msg){
	fprintf(stderr, "%s", msg);
	exit(1);
}

