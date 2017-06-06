#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>



/* 模拟客户端发送http请求 */



int main(int argc, char *argv[])
{
    int sockfd;
    int len;
    struct sockaddr_in address;
    int result;
    char str[1024];
	char str2[1024];
	char str3[1024];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(8899);
    len = sizeof(address);
    result = connect(sockfd, (struct sockaddr *)&address, len);

    if (result == -1)
    {
        perror("oops: client1");
        return 0;
    }
	strcpy(str, "GET / HTTP/1.1\r\n");
    write(sockfd, str, strlen(str));
	
	
	//为了测试状态机, 暂停， 继续发送
	sleep(1);

	strcpy(str2, "Host:127.0.0.1\r\n");
    write(sockfd, str2, strlen(str2));

	sleep(1);

	strcpy(str3, "\r\n");
    write(sockfd, str3, strlen(str3));

    close(sockfd);
    return 0;
}
