#include "http.h"

int main(int argc, char *argv[]){
	
	if(argc != 2){
		err_user("Usage: ./studyHttpd <ip address>\n");
	}

	int sin_size;
	int sockfd, *client_fd;
	struct sockaddr_in server_sock, client_sock;

	/* 线程池初始化 */
	if(-1 == pool_init(MAX_POOL_SIZE)) exit(1);

	/* 建立套接字, 绑定sockaddr信息, 监听... */
	if(-1 == (sockfd = init_server())) exit(1);

	/* 退出 */
	//signal(SIGINT, handle_exit);
	
	/* 浏览器取消请求 */
	//signal(SIGPIPE, cancel_request);

	sin_size = sizeof(client_sock);

	while(1){

		/* 防止accept和传进线程的client_fd赋值语句存在竞争 */
		if(NULL == (client_fd = (int *)malloc(sizeof(int)))){
			err_sys("client_fd alloc memory fail", DEBUGPARAMS);
			break;	
		}

		if(-1 == (*client_fd = accept(sockfd, (struct sockaddr *) &client_sock, &sin_size))){
			err_sys("accept() fail", DEBUGPARAMS);
			break;
		}
		/* 线程处理客户端连接 */	
		if(-1 == add_job(handle_request, client_fd)) break;
	}
	
	/* 退出服务 */
	end_server(sockfd);

    return 0;
}

