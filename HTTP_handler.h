#ifndef HTTP_HANDLER
#define HTTP_HANDLER

#include<functional>
#include<string>
#include<algorithm>
#include<array>
#include<unordered_map>
#include<utility>
#define _STD ::std::

namespace http_handler{

enum METHOD
{
	UNKNOW ,
	GET ,
	POST ,
	HEAD ,
	PUT ,
	DELETE ,
	TRACE ,
	OPTIONS ,
	CONNECT
};

enum  CONNECTION
{
	CLOSE ,
	KEEP_ALIVE ,
	UPGRADE,
	CONNECTION_UNKNOW
};

class Request
{
public:
	static Request from_string(char* buf , unsigned size);

	//use like this ,you also need to #include<functional>
	//_STD function<int (void*,size_t)>  func = _STD bind(read , connfd,_STD placeholders::_1,_STD placeholders::_2);
	static Request from_function(_STD function<int (void* , size_t)>  func);

	Request()=delete;

	Request(char* buf , unsigned size);

	Request(_STD function<int (void* , size_t)>  func);

	Request(Request&& right);

	METHOD get_method() const;

	CONNECTION get_content_type() const;

	int get_content_length() const;

	_STD pair<int , int> get_http_version() const;

	_STD string get_filepath() const;

	_STD string get_cookie() const;

	_STD string get_args() const;

	_STD string get_field(_STD string name) const;

private:
	int parse_requestline(char* from , char* last);

	void parse_header(char* from , char* last);

	int parse_method(char* from , char* last);

	int recv_a_line(_STD function<int (void* , size_t)>  func , char* buf);

	void _init();

	METHOD method_;
	CONNECTION connection_;
	_STD string filepath_;
	_STD string args_;
	_STD string cookie_;
	int http_version_major_;
	int http_version_minor_;
	int content_length_;
	_STD unordered_map<_STD string , _STD string> fields_;
};

class Response
{
public:
	Response();

	void set_version(int major,int minor);

	void set_connection(CONNECTION c);

	void set_content_length_(int n);

	void set_status_code_(int code);

	bool set_fields(const _STD string& name , const _STD string& value);

	int set_content(const _STD string& content);

	int set_content(_STD string&& content);

	//use like this ,you also need to #include<funtional>
	//_STD function<int (void* , size_t)> func = _STD bind(write , connfd , _STD placeholders::_1 , _STD placeholders::_2);
	int write(_STD function<int (void* , size_t)> func) const;

private:
	int make_status_code_(char* dest) const;

	int http_version_major_;
	int http_version_minor_;
	CONNECTION connection_;
	int status_code_;
	int content_length_;
	_STD unordered_map<_STD string , _STD string> fields_;
	_STD string content_;
};

} // namespace http_handler

#endif // !HTTP_HANDLER
