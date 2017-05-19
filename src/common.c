#include "common.h"




void err_sys(const char *msg){
	perror(msg);
	exit(1);
}

void err_user(const char *msg){
	fprintf(stderr, "%s", msg);
	exit(1);
}

void err_msg(const char *msg){
	fprintf(stderr, "%s", msg);
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
