#include "common.h"

void err_sys(const char *msg, const char *filename, int line, const char *func){
	fprintf(stderr, "%s(%d)-%s error --> %s : %s\n", filename, line, func, msg, strerror(errno));
}

void err_user(char *msg){
	fprintf(stderr, "%s", msg);
}


/* 信号处理, 成功返回1, 失败返回-1 */
int init_signal(){
	struct sigaction act;
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	//浏览器取消请求会导致产生SIGPIPE信号， 忽略之
	if(-1 == sigaction(SIGPIPE, &act, NULL)){
		err_sys("signal SIGPIPE fail", DEBUGPARAMS);
		return -1;
	}
	return 1;
}

int buffer_path_simplify(char *dest, char *src){  
	int count;  
	char c;  
	char *start, *slash, *walk, *out;  
	char pre; //当前匹配的前一个字符  
					  
	if (src == NULL ||  dest == NULL)  
		return -1;  
						  
	walk  = src;  
	start = dest;  
	out   = dest;  
	slash = dest;  
										  
	while (*walk == ' ') {  
		walk++;  
	}  
											  
	pre = *(walk++);  
	c    = *(walk++);  
	if (pre != '/') {  
		*(out++) = '/';  
	}  
	*(out++) = pre;  
															  
	if (pre == '\0') {  
		*out = 0;  
		return 0;  
	} 
	
    while (1) {  
		if (c == '/' || c == '\0') {  
			count = out - slash;  
			if (count == 3 && pre == '.') {  
				out = slash;  
				if (out > start) {  
					out--;  
					while (out > start && *out != '/') {  
						out--;  
					}  
				}  
				if (c == '\0')  
					out++;  
			} else if (count == 1 || pre == '.') {  

				out = slash;  
				if (c == '\0')  
					out++;  
			}  
			slash = out;  
		}  

		if (c == '\0')  
			break;   
		pre = c;  
		c    = *walk;  
		*out = pre;  
		out++;  
		walk++;  
	}  
	 *out = '\0';  
	 return 0;  
}
