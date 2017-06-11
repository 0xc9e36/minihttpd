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
	/* 
	 *	浏览器端关闭, 第一次往里write数据, 返回RST报文, 第二次write则产生SIGPIPE信号, 这里将其忽略
	 */
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


int  load_config(config_t *config){
	
	char src[50];
	sprintf(src, "%shttp.conf", ROOT);

	FILE *fp;
	char line[512];
	
	if(NULL == (fp = fopen(src, "r"))){
		err_sys("load config file error", DEBUGPARAMS);
		return -1;
	}

	int len, i;
	char *val;
	while(fgets(line, 512, fp)){
		
		i = 0;
		//去掉空格
		while(line[i] == ' ') i++;
		if(line[i] == '\n' || line[i] == '#') continue;
	
		val = strstr(line, "=");
		if(!val) return -1;
		val++;

		if(val[strlen(val) - 1] == '\n') val[strlen(val) - 1] = '\0';

		if(strncmp(line, "port", 4) == 0) config->port = atoi(val);
		else if(strncmp(line, "thread_num", 10) == 0)	config->thread_num = atoi(val);

	}

	printf("------载入配置文件------\nport:%d\nthread num:%d\n",config->port, config->thread_num);

	fclose(fp);
	return 1;
}
