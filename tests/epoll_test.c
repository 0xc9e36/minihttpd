#include <string.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <sys/select.h>  
#include <sys/time.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <sys/epoll.h>  
#include <errno.h>  
#define OPEN_MAX 100  
  
int main(int argc, char *argv[])  
{  
    struct epoll_event event;   // 告诉内核要监听什么事件    
    struct epoll_event wait_event; //内核监听完的结果  
      
    //1.创建tcp监听套接字  
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);  
      
    //2.绑定sockfd  
    struct sockaddr_in my_addr;  
    bzero(&my_addr, sizeof(my_addr));  
    my_addr.sin_family = AF_INET;  
    my_addr.sin_port = htons(8001);  
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
    bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));  
      
    //3.监听listen  
    listen(sockfd, 10);  
       
    //4.epoll相应参数准备  
    int fd[OPEN_MAX];  
    int i = 0, maxi = 0;  
    memset(fd,-1, sizeof(fd));  
    fd[0] = sockfd;  
      
    int epfd = epoll_create(10); // 创建一个 epoll 的句柄，参数要大于 0， 没有太大意义    
    if( -1 == epfd ){    
        perror ("epoll_create");    
        return -1;    
    }    
        
    event.data.fd = sockfd;     //监听套接字    
    event.events = EPOLLIN; // 表示对应的文件描述符可以读  
      
    //5.事件注册函数，将监听套接字描述符 sockfd 加入监听事件    
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);    
    if(-1 == ret){    
        perror("epoll_ctl");    
        return -1;    
    }   
      
    //6.对已连接的客户端的数据处理  
    while(1)  
    {  
        // 监视并等待多个文件（标准输入，udp套接字）描述符的属性变化（是否可读）    
        // 没有属性变化，这个函数会阻塞，直到有变化才往下执行，这里没有设置超时     
        ret = epoll_wait(epfd, &wait_event, maxi+1, -1);   
          
        //6.1监测sockfd(监听套接字)是否存在连接  
        if(( sockfd == wait_event.data.fd )     
            && ( EPOLLIN == wait_event.events & EPOLLIN ) )  
        {  
            struct sockaddr_in cli_addr;  
            int clilen = sizeof(cli_addr);  
              
            //6.1.1 从tcp完成连接中提取客户端  
            int connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);  
              
            //6.1.2 将提取到的connfd放入fd数组中，以便下面轮询客户端套接字  
            for(i=1; i<OPEN_MAX; i++)  
            {  
                if(fd[i] < 0)  
                {  
                    fd[i] = connfd;  
                    event.data.fd = connfd; //监听套接字    
                    event.events = EPOLLIN; // 表示对应的文件描述符可以读  
                      
                    //6.1.3.事件注册函数，将监听套接字描述符 connfd 加入监听事件    
                    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);    
                    if(-1 == ret){    
                        perror("epoll_ctl");    
                        return -1;    
                    }   
                      
                    break;  
                }  
            }  
              
            //6.1.4 maxi更新  
            if(i > maxi)  
                maxi = i;  
                  
            //6.1.5 如果没有就绪的描述符，就继续epoll监测，否则继续向下看  
            if(--ret <= 0)  
                continue;  
        }  
          
        //6.2继续响应就绪的描述符  
        for(i=1; i<=maxi; i++)  
        {  
            if(fd[i] < 0)  
                continue;  
              
            if(( fd[i] == wait_event.data.fd )     
            && ( EPOLLIN == wait_event.events & (EPOLLIN|EPOLLERR) ))  
            {  
                int len = 0;  
                char buf[128] = "";  
                  
                //6.2.1接受客户端数据  
                if((len = recv(fd[i], buf, sizeof(buf), 0)) < 0)  
                {  
                    if(errno == ECONNRESET)//tcp连接超时、RST  
                    {  
                        close(fd[i]);  
                        fd[i] = -1;  
                    }  
                    else  
                        perror("read error:");  
                }  
                else if(len == 0)//客户端关闭连接  
                {  
                    close(fd[i]);  
                    fd[i] = -1;  
                }  
                else//正常接收到服务器的数据  
                    send(fd[i], buf, len, 0);  
                  
                //6.2.2所有的就绪描述符处理完了，就退出当前的for循环，继续poll监测  
                if(--ret <= 0)  
                    break;  
            }  
        }  
    }  
    return 0;  
}  
