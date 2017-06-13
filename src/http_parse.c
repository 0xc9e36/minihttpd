#include "http_parse.h"

/* 解析http请求行 */
int http_parse_line(http_request *hr){
	

	if(hr->alalyzed != 0) return HTTP_OK;

	size_t index;
	u_char ch, *cur, *m;

	/* 用一个状态机维持请求头, 类似 GET /index.html HTTP/1.1\r\n   */
	enum{
		start = 0,
		method_start,
		space_before_url,
		url_start,
		http_start,
		http_h,
		http_ht,
		http_htt,
		http_http,
		first_major_digit,
		major_digit,
		first_minor_digit,
		minor_digit,
		http_r,
	}state;

	state = hr->state;
	for(index = hr->pos; index < hr->last; index++){
	
		cur = (u_char *)&(hr->buf[index % HTTP_BUF_SIZE]);
		ch = *cur;
	
		switch(state){
		
			case start:
				hr->request_start = cur;

				if(ch == '\r' || ch == '\n') break;
				
				if((ch < 'A' || ch > 'Z')) return HTTP_LINE_ERROR;
				
				state = method_start;
				break;
			
			case method_start:
				if(ch == ' '){
					// GET /
					hr->method_end = cur;
					m = hr->request_start;
					
					switch(cur - m){
						case 3:
							if(0 == strncmp(hr->request_start, "GET", 3)) hr->method = GET;
							break;
						
						case 4:
							if(0 == strncmp(hr->request_start, "POST", 4)) hr->method = POST;
							break;
						
						default :
							hr->method = UNKNOW;
							break;
						
					}

					state = space_before_url;
					break;
				}

				if((ch < 'A' || ch > 'Z')) return HTTP_LINE_ERROR;
				break;

			case space_before_url:
				if(ch == '/'){
					hr->url_start = cur;
					state = url_start;
					break;
				}
				
				switch(ch){
					case ' ': break;
					default : return HTTP_LINE_ERROR;
				}
				break;

			case url_start:
				switch(ch){
					case ' ':
						hr->url_end = cur;
					
						memset(hr->url, '\0', sizeof(char)*1024);
						strncpy(hr->url, hr->url_start, hr->url_end - hr->url_start);
						
						state = http_start;
						break;
					default	: break;
				}
				break;

			case http_start:
				switch(ch){
					case ' ':
						break;

					case 'H':
						state = http_h;
						break;
					default :
						return HTTP_LINE_ERROR;
				}
				break;

			case http_h:
				switch(ch){
					case 'T':
						state = http_ht;
						break;
					default :
						return HTTP_LINE_ERROR;
				}
				break;

			case http_ht:
				switch(ch){
					case 'T':
						state = http_htt;
						break;

					default:
						return HTTP_LINE_ERROR;
				}
				break;

			case http_htt :
				switch(ch){
					case 'P':
						state = http_http;
						break;
					default:
						return HTTP_LINE_ERROR;
				}
				break;
			
			case http_http:
				switch(ch){
					
					case '/':
						state =  first_major_digit;
						break;
					default :
						return HTTP_LINE_ERROR;
				}
				break;
			
			case first_major_digit:
				if(ch < '1' || ch > '9') return HTTP_LINE_ERROR;
				hr->http_major = ch - '0';
				state = major_digit;
				break;
				
			case major_digit:
				switch(ch){
					case '.':
						state = first_minor_digit;
						break;
					default:
						return HTTP_LINE_ERROR;
				}
				break;
				
			case first_minor_digit:
				if(ch < '1' || ch > '9') return HTTP_LINE_ERROR;
				hr->http_minor = ch - '0';
				hr->request_end = cur;
				state = minor_digit;
				break;

			case minor_digit:

				switch(ch){
					case '\r':
						state = http_r;
						break;
					default :
						return HTTP_LINE_ERROR;
				}
				break;

			case http_r:
				switch(ch){
					case '\n':
						hr->pos = index + 1;
						
						if(hr->request_end == NULL) hr->request_end = cur;
						hr->state = start;
						hr->alalyzed = 1;
						return HTTP_OK;
					default:
						return HTTP_LINE_ERROR;
				}
		}
	}

	hr->pos = index;
	hr->state = state;
	return HTTP_AGAIN;
}


/* 解析http请求 */
int http_parse_head(http_request *hr){

	if(hr->alalyzed != 1) return HTTP_OK;

	u_char ch, *cur;
	size_t index;
	
	list_head *pos;
	enum{
		start = 0,
		key_start,
		space_before_colon,
		space_before_val,
		val_start,
		cr,
		lf,
		crlf,
	}state;

	state = hr->state;
	http_header *hd;

	//printf("解析请求头开始\n");
	for(index = hr->pos; index < hr->last; index++){
		
		cur = (u_char *)&(hr->buf[index % HTTP_BUF_SIZE]);
		ch = *cur;
	
		switch(state){
			case start:
				if(ch == '\r' || ch == '\n') break;
				hr->key_start = cur;
				state = key_start;
				break;

			case key_start:
				if(ch == ' '){
					//当前读取到的key
					hr->key_end = cur;
					state = space_before_colon;
					break;
				}

				//key读取完毕
				if(ch == ':'){
					hr->key_end = cur;
					state = space_before_val;
					break;
				}

				break;

			case space_before_colon:
				if(ch == ' '){
					break;
				}else if(ch == ':'){
					state = space_before_val;
					break;		
				}else{
					return HTTP_HEADER_ERROR;
				}

			case space_before_val:
				if(ch == ' ') break;
				state = val_start;
				hr->val_start = cur;
				break;

			case val_start:
				if(ch == '\r'){
					hr->val_end = cur;
					state = cr;
				}
				
				if(ch == '\n'){
					hr->val_end = cur;
					state = lf;
				}

				break;

			case cr:
				if(ch == '\n'){

					//放到链表里面保存头信息
					state = lf;
					hd = (http_header *)malloc(sizeof(http_header));
					hd->key_start = hr->key_start;
					hd->key_end = hr->key_end;

					hd->val_start = hr->val_start;
					hd->val_end = hr->val_end;

					/* 获取 Content-Length字段 */
					if(strncmp("Content-Length", hd->key_start, hd->key_end - hd->key_start) == 0){
						char val[20];
						memset(val, '\0', sizeof(char) * 20);
						strncpy(val, hd->val_start, hd->val_end - hd->val_start);
						hr->conlength = atoi(val);
					}
				
					list_add(&(hd->list), &(hr->list));
					break;
				}else{
					return HTTP_HEADER_ERROR;
				}

			case lf:
				if(ch == '\r'){ //空行
					state = crlf;
				}else{	//下一个请求头
					hr->key_start = cur;
					state = key_start;
				}
				break;

			case crlf:
				switch(ch){
					case '\n' :
						hr->pos = index + 1;
						hr->state = start;
						hr->alalyzed = 2;
						return HTTP_OK;
					default :
						return HTTP_HEADER_ERROR;
				}
				break;
		}	

	}

	hr->pos = index;
	hr->state = state;
	return HTTP_AGAIN;
}



int http_parse_body(http_request *hr){
	if(hr->alalyzed != 2 || hr->conlength == 0){
		return HTTP_OK;
	}
	size_t index;
	u_char *cur;

	index = hr->pos;
	cur = (u_char *)&(hr->buf[index % HTTP_BUF_SIZE]);
	if(hr->read_length == 0){
		hr->content = (char *)malloc(sizeof(char) * hr->conlength + 12);
	}
	
	int len;
	char *buf = (char *)malloc(sizeof(char) * (HTTP_BUF_SIZE + 1));
	len =  hr->last - index;

	memcpy(buf, cur, len);

	mystrcat(hr->content, hr->read_length,  buf, len);

	hr->read_length += len;

	if(hr->read_length >= hr->conlength){
						
		hr->alalyzed = 3;
		return HTTP_OK;
	}
	
	free(buf);

	hr->pos = hr->last;

	return HTTP_AGAIN;
}
