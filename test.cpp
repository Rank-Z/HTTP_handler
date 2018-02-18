#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<string.h>
#include"HTTP_handler.h"
#include<functional>

using namespace http_handler;


//test on Linux
int main()
{
	int listenfd = socket(AF_INET , SOCK_STREAM , 0);
	sockaddr_in servaddr;
	memset(&servaddr , 0 , sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8000);
	bind(listenfd , (sockaddr*)&servaddr , sizeof(servaddr));
	listen(listenfd , 10);
	int connfd = accept(listenfd , NULL , NULL);

	char buf [1024];
	//_STD function<int (void*,size_t)>  func1 = _STD bind(read , connfd,_STD placeholders::_1,_STD placeholders::_2);
	//Request::from_function(func1);
	int size = recv(connfd , buf , 1024 , 0);
	Request re(buf , size);

	Response rs;
	rs.set_connection(CLOSE);
	rs.set_content("<html>bad</html>");
	rs.set_version(1,0);
	rs.set_status_code_(404);

	_STD function<int (void* , size_t)> func2 = _STD bind(write , connfd , _STD placeholders::_1 , _STD placeholders::_2);
	rs.write(func2);

	return 1;
}