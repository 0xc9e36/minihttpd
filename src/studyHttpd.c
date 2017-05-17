#include "http.h"
#include "common.h"

int main(int argc, char *argv[]){
	
	if(argc != 2){
		err_user("Usage: ./studyHttpd <ip address>\n");
	}

	int sin_size;
	int sockfd, client_fd;
	struct sockaddr_in server_sock, client_sock;

	/* 线程池初始化 */
	pool_init(MAX_POOL_SIZE);

	sockfd = init_server();

	sin_size = sizeof(client_sock);
	while(1){
	
		if(-1 == (client_fd = accept(sockfd, (struct sockaddr *) &client_sock, &sin_size))) err_sys("accept");
		
		add_job(handle_request, &client_fd);
	}
	close(sockfd);
    return 0;
}

