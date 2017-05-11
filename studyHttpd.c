#include "http.h"

int main(int argc, char *argv[]){
	
	if(argc != 2){
		err_user("Usage: ./studyHttpd <ip address>\n");
	}

	pthread_t ntid;
	int sin_size;
	int sockfd, client_fd;
	struct sockaddr_in server_sock, client_sock;

	sockfd = init_server();

	sin_size = sizeof(client_sock);
	while(1){
	
		if(-1 == (client_fd = accept(sockfd, (struct sockaddr *) &client_sock, &sin_size))) err_sys("accept");
		
		if(pthread_create(&ntid, NULL, (void *)handle_request, &client_fd) != 0) err_sys("pthread_create");
	}
	close(sockfd);
    return 0;
}

