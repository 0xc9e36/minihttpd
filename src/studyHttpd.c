#include "http.h"

int main(int argc, char *argv[]){
	
	if(argc != 2){
		err_user("Usage: ./studyHttpd <ip address>\n");
	}

	int sin_size;
	int sockfd, client_fd;
	struct sockaddr_in server_sock, client_sock;

	/* 线程池初始化 */
	if(-1 == pool_init(MAX_POOL_SIZE)) exit(1);

	/* 建立套接字, 绑定sockaddr信息, 监听... */
	if(-1 == (sockfd = init_server())) exit(1);

	
	sin_size = sizeof(client_sock);

	while(1){
	
		if(-1 == (client_fd = accept(sockfd, (struct sockaddr *) &client_sock, &sin_size))){
			err_sys("accept() fail", DEBUG);
			break;
		}
		/* 线程处理客户端连接 */	
		if(-1 == add_job(handle_request, &client_fd)) break;
	}
	close(sockfd);
	pool_destroy();
    return 0;
}

