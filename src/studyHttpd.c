#include "http.h"
#include "epoll.h"

int main(int argc, char *argv[]){
	
	if(argc != 2){
		err_user("Usage: ./studyHttpd <ip address>\n");
	}

	int sin_size, i;
	int sockfd;
	struct sockaddr_in server_sock, client_sock;
	struct epoll_event event;

	/* 线程池初始化 */
	if(-1 == pool_init(MAX_POOL_SIZE)) exit(1);

	/* 信号处理 */
	if(-1 == init_signal()){
		pool_destroy();
		exit(1);
	}
	/* 建立套接字, 绑定sockaddr信息, 监听... */
	if(-1 == (sockfd = init_server())){
		pool_destroy();
		exit(1);
	}
	
	/* 设置非阻塞I/O */
	if(-1 == set_non_blocking(sockfd)){
		close(sockfd);
		pool_destroy();
		exit(1);
	};

	int epfd;
	/* epoll 初始化*/
	if(-1 == (epfd = epoll_init(0))){	
		close(sockfd);
		pool_destroy();
	}

	http_request *request = (http_request *)malloc(sizeof(http_request));
	if(!request){	
		close(sockfd);
		pool_destroy();
	}

	init_http_request(request, sockfd, epfd);

	event.data.ptr = (void *)request;
	event.events  = EPOLLIN | EPOLLET;

	/* 添加监听 */
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
	if(-1 == ret){
		err_sys("epoll_ctl fail", DEBUGPARAMS);
		exit(0);
	}

	

	sin_size = sizeof(client_sock);

	while(1){

		ret = epoll_wait(epfd, events, MAX_FD, -1);
		
		/* 遍历 */
		for(i = 0; i < ret; i++){
			http_request *hr = (http_request *)events[i].data.ptr;
			
			/* 检测套接字是否存在连接*/
			if(sockfd == hr->sockfd){
				int client_fd;

				while(1){
					if(-1 == (client_fd = accept(sockfd, (struct sockaddr *) &client_sock, &sin_size))){
						if((errno == EAGAIN) || (errno == EWOULDBLOCK)){
							break;
						}else{	
							err_sys("accept() fail", DEBUGPARAMS);
							break;
						}
					}
					set_non_blocking(client_fd);
					http_request *request = (http_request *)malloc(sizeof(http_request));
					init_http_request(request, client_fd, epfd);

					event.data.ptr = (void *)request;
					event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;  //可读 + 边沿触发 + 避免分配给多个线程
					if(-1 == (ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event))){
						err_sys("epoll_ctl fail", DEBUGPARAMS);
						break;
					}
				}
			}else{
				if((events[i].events & EPOLLERR) || events[i].events & EPOLLHUP || (!(events[i].events & EPOLLIN))){
					close(hr->sockfd);
					continue;
				}
				if(-1 == add_job(handle_request, events[i].data.ptr)) break;
			}
		}
	}
	
	
	/* 退出服务 */
	close(sockfd);
	pool_destroy();

    return 0;
}

		/* 防止accept和传进线程的client_fd赋值语句存在竞争 */
		//if(NULL == (client_fd = (int *)malloc(sizeof(int)))){
		//	err_sys("client_fd alloc memory fail", DEBUGPARAMS);
		//	break;	
		//}

		//if(-1 == (*client_fd = accept(sockfd, (struct sockaddr *) &client_sock, &sin_size))){
		//	err_sys("accept() fail", DEBUGPARAMS);
		//	break;
		//}
		/* 线程处理客户端连接 */	
		//if(-1 == add_job(handle_request, client_fd)) break;
