#include"HTTP_handler.h"
#include<string.h>

namespace http_handler
{


Request Request::from_string(char* buf , unsigned size)
{
	Request ret(buf , size);
	return ret;
}

Request Request::from_function(_STD function<int (void* , size_t)>  func)
{
	Request ret(func);
	return ret;
}


Request::Request(char* buf , unsigned size)
{
	static const _STD string end_of_line("\r\n");
	unsigned from = 0;
	unsigned last = 0;
	for (;;)
	{
		last = _STD search(&buf [from] , &buf [size] , end_of_line.cbegin() , end_of_line.cend()) - buf;
		if (last == size || last == from)
			break;

		if (from == 0)
		{
			if (parse_requestline(buf , &buf [last]) < 0)
				return;
		}
		else
		{
			parse_header(&buf [from] , &buf [last]);
		}
		from = last + 2;
	}
	_init();
}

Request::Request(_STD function<int (void* , size_t)>  func)
{
	char buffer [256];
	int cur = 0;
	bool first = true;
	for (;;)
	{
		cur = recv_a_line(func , buffer);
		if (cur == -1)
			break;
		if (cur == 2)
			break;
		if (first)
		{
			parse_requestline(buffer , &buffer [cur - 2]);
			first = false;
		}
		else
			parse_header(buffer , &buffer [cur - 2]);
	}
	_init();
}

Request::Request(Request&& right)
	:method_(_STD move(right.method_)),connection_(_STD move(right.connection_)),
	filepath_(_STD move(right.filepath_)),args_(_STD move(right.args_)),
	http_version_major_(right.http_version_major_),http_version_minor_(right.http_version_minor_),
	content_length_(_STD move(right.content_length_)),
	fields_(_STD move(right.fields_))
{   }

METHOD Request::get_method() const
{
	return method_;
}

CONNECTION Request::get_content_type() const
{
	return connection_;
}

int Request::get_content_length() const
{
	return content_length_;
}

_STD pair<int , int> Request::get_http_version() const
{
	return _STD make_pair(http_version_major_ , http_version_minor_);
}

_STD string Request::get_filepath() const
{
	return filepath_;
}

_STD string Request::get_args() const
{
	return args_;
}

_STD string Request::get_cookie() const
{
	return cookie_;
}

_STD string Request::get_field(_STD string name) const
{
	auto it = fields_.find(name);
	if (it == fields_.cend())
		return _STD string();
	else
		return (*it).second;
}

int Request::parse_requestline(char* from , char* last)
{
	char* cur = _STD find(from , last , ' ');
	if (cur == last)
		return -1;
	if (parse_method(from , cur) < 0)
		return -1;
	from = cur + 1;
	cur = _STD find(from , last , '?');
	if (cur != last)
	{
		*cur = '\0';
		filepath_ = from;
		from = cur + 1;
		cur = _STD find(from , last , ' ');
		if (cur == last)
			return -1;
		*cur = '\0';
		args_ = from;
	}
	else //can't find '?'
	{
		cur = _STD find(from , last , ' ');
		if (cur == last)
			return -1;
		*cur = '\0';
		filepath_ = from;
	}
	from = cur + 1;
	cur = _STD find(from , last , '/');
	if (cur == last)
		return -1;
	++cur;
	*last = '\0';
	float temp = atof(cur);
	if (temp < 1.9)
	{
		http_version_major_ = 1;
		if (temp < 1.09)
			http_version_minor_ = 0;
		else
			http_version_minor_ = 1;
	}
	else
	{
		http_version_major_ = 2;
		http_version_minor_ = 0;
	}
	return 1;
}

void Request::parse_header(char* from , char* last)
{
	char* mid = _STD find(from , last , ':');
	if (mid == last)
		return;
	fields_.emplace(_STD move(_STD string(from , mid)) , _STD move(_STD string(mid + 2 , last)));
}

int Request::parse_method(char* from , char* last)
{
	static const _STD array<_STD string , 9> method_arr =
	{ "GET","POST","HEAD","PUT","DELETE","TRACE","OPTIONS","CONNECT" };
	_STD string input(from , last);
	auto it = _STD find(method_arr.cbegin() , method_arr.cend() , input);
	if (it == method_arr.cend())
	{
		method_ = UNKNOW;
		return -1;
	}
	method_ = static_cast<METHOD>((it - method_arr.cbegin()) + 1);
	return 1;
}

int Request::recv_a_line(_STD function<int (void* , size_t)>  func , char* buf)
{
	int ret = 0;
	int now = 0;
	bool read_double = true;
	for (;;)
	{
		if (read_double)
		{
			now = func(&buf [ret] , 2);
			if (now < 2)
				return -1;
			if (buf [ret] == '\r'&&buf [ret + 1] == '\n')
				return ret + 2;
			else if (buf [ret + 1] == '\r')
				read_double = false;
			ret += 2;
		}
		else
		{
			now = func(&buf [ret] , 1);
			if (now < 1)
				return -1;
			if (buf [ret] == '\n')
				return 1;
			else
				++ret;
		}
		if (ret > 254)
			return -1;
	}
}

void Request::_init()
{
	auto it = fields_.find(_STD string("Content-Length"));
	if (it == fields_.cend())
		content_length_ = 0;
	else
		content_length_ = _STD stoi((*it).second);

	it = fields_.find(_STD string("Connection"));
	if (it == fields_.cend())
	{
		if ((http_version_major_==1)&&(http_version_minor_==0))
			connection_ = CLOSE;
		else
			connection_ = KEEP_ALIVE;
	}
	else
	{
		if (_STD equal((*it).second.cbegin() , (*it).second.cend() , "keep-alive"))
			connection_ = KEEP_ALIVE;
		else if (_STD equal((*it).second.cbegin() , (*it).second.cend() , "Upgrade"))
			connection_ = UPGRADE;
		else
			connection_ = CLOSE;
	}

	it = fields_.find(_STD string("Cookie"));
	if (it != fields_.end())
		cookie_ = (*it).second;
}

Response::Response()
	:http_version_major_(1),http_version_minor_(0),
	connection_(CLOSE),content_length_(0),status_code_(200)
{   }

void Response::set_version(int major,int minor)
{
	http_version_major_ = major;
	http_version_minor_ = minor;
}


void Response::set_connection(CONNECTION c)
{
	connection_ = c;
}

void Response::set_content_length_(int n)
{
	content_length_ = n;
}

void Response::set_status_code_(int code)
{
	status_code_ = code;
}

bool Response::set_fields(const _STD string& name , const _STD string& value)
{
	auto it = fields_.find(name);
	if (it == fields_.end())
	{
		fields_.emplace(name , value);
		return true;
	}
	else
	{
		(*it).second = value;
		return false;
	}
}

int Response::set_content(const _STD string& content)
{
	content_ = content;
	content_length_ = content_.size();
	return content_length_;
}

int Response::set_content(_STD string&& content)
{
	content_ = _STD move(content);
	content_length_ = content_.size();
	return content_length_;
}

int Response::write(_STD function<int (void* , size_t)> func) const
{
	char buffer [32] = "HTTP/";
	if (http_version_major_ == 1)
	{
		if (http_version_minor_ == 0)
			strcpy(&buffer [5] , "1.0 ");
		else if (http_version_minor_ == 1)
			strcpy(&buffer [5] , "1.1 ");
		else
			return -1;
	}
	else if (http_version_major_ == 2)
	{
		if (http_version_minor_ == 0)
			strcpy(&buffer [5] , "2.0 ");
		else
			return -1;
	}
	else
		return -1;

	if (make_status_code_(&buffer [9]) < 0)
		return -1;
	func(buffer , strlen(buffer));

	if (content_length_ >= 0)
	{
		int temp=sprintf(buffer , "Content-Length: %d\r\n" , content_length_);
		buffer [temp] = '\0';
		func(buffer , strlen(buffer));
	}

	if (connection_ == CLOSE)
	{
		strcpy(buffer , "Connection: close\r\n");
	}
	else if(connection_==KEEP_ALIVE)
	{
		strcpy(buffer , "Connection: keep-alive\r\n");
	}
	else
	{
		strcpy(buffer , "Connection: Upgrade\r\n");
	}
	func(buffer , strlen(buffer));

	_STD string str_temp;
	static const _STD string sparator(": ");
	static const _STD string end_of_line("\r\n");
	for (auto &t : fields_)
	{
		str_temp += t.first;
		str_temp += sparator;
		str_temp += t.second;
		str_temp += end_of_line;
	}
	str_temp += end_of_line;
	func((void*)&(*(str_temp.begin())) , str_temp.size());

	if (!content_.empty())
	{
		func((void*)&(*(content_.begin())) , content_.size());
	}

	return 1;
}

int Response::make_status_code_(char* dest) const
{
	switch (status_code_)
	{
	case 100:
		strcpy(dest , "100 Continue\r\n");
		break;
	case 101:
		strcpy(dest , "101 Switchind Protocols\r\n");
		break;
	case 102:
		strcpy(dest , "102 Processing\r\n");
		break;
	case 200:
		strcpy(dest , "200 OK\r\n");
		break;
	case 201:
		strcpy(dest , "201 Created\r\n");
		break;
	case 202:
		strcpy(dest , "202 Accepted\r\n");
		break;
	case 203:
		strcpy(dest , "203 Non-Authoritative Information\r\n");
		break;
	case 204:
		strcpy(dest , "204 No Content\r\n");
		break;
	case 205:
		strcpy(dest , "205 Reset Content\r\n");
		break;
	case 206:
		strcpy(dest , "206 Partial Content\r\n");
		break;
	case 207:
		strcpy(dest , "207 Multi-Status\r\n");
		break;
	case 208:
		strcpy(dest , "208 Already Reported\r\n");
		break;
	case 226:
		strcpy(dest , "226 IM Used\r\n");
		break;
	case 300:
		strcpy(dest , "300 Multiple Choices\r\n");
		break;
	case 301:
		strcpy(dest , "301 Moved Permanently\r\n");
		break;
	case 302:
		strcpy(dest , "302 Found\r\n");
		break;
	case 303:
		strcpy(dest , "303 See Other\r\n");
		break;
	case 304:
		strcpy(dest , "304 Not Modified\r\n");
		break;
	case 305:
		strcpy(dest , "305 Use Proxy\r\n");
		break;
	case 306:
		strcpy(dest , "306 Switch Proxy\r\n");
		break;
	case 307:
		strcpy(dest , "307 Temporary Redirect\r\n");
		break;
	case 308:
		strcpy(dest , "308 Permanent Redirect\r\n");
		break;
	case 400:
		strcpy(dest , "400 Bad Request\r\n");
		break;
	case 401:
		strcpy(dest , "401 Unauthorized\r\n");
		break;
	case 402:
		strcpy(dest , "402 Payment Required\r\n");
		break;
	case 403:
		strcpy(dest , "403 Forbidden\r\n");
		break;
	case 404:
		strcpy(dest , "404 Not Found\r\n");
		break;
	case 405:
		strcpy(dest , "405 Method Not Allowed\r\n");
		break;
	case 406:
		strcpy(dest , "406 Not Acceptable\r\n");
		break;
	case 407:
		strcpy(dest , "407 Proxy Authentication Required\r\n");
		break;
	case 408:
		strcpy(dest , "408 Request Timeout\r\n");
		break;
	case 409:
		strcpy(dest , "409 Conflict\r\n");
		break;
	case 410:
		strcpy(dest , "410 Gone\r\n");
		break;
	case 411:
		strcpy(dest , "411 Length Required\r\n");
		break;
	case 412:
		strcpy(dest , "412 Precondition Failed\r\n");
		break;
	case 413:
		strcpy(dest , "413 Request Entity Too Large\r\n");
		break;
	case 414:
		strcpy(dest , "414 Request-URI Too Long\r\n");
		break;
	case 415:
		strcpy(dest , "Unsupported Media Type\r\n");
		break;
	case 416:
		strcpy(dest , "416 Requested Range Not Satisfiable\r\n");
		break;
	case 417:
		strcpy(dest , "417 Expectation Failed\r\n");
		break;
	case 418:
		strcpy(dest , "418 I'm a teapot\r\n");
		break;
	case 420:
		strcpy(dest , "420 Enhance Your Caim\r\n");
		break;
	case 421:
		strcpy(dest , "421 Misdirected Request\r\n");
		break;
	case 422:
		strcpy(dest , "422 Unprocessable Entity\r\n");
		break;
	case 423:
		strcpy(dest , "423 Locked\r\n");
		break;
	case 425:
		strcpy(dest , "425 Unodered Cellection\r\n");
		break;
	case 426:
		strcpy(dest , "426 Upgrade Required\r\n");
		break;
	case 428:
		strcpy(dest , "428 Precondition Required\r\n");
		break;
	case 429:
		strcpy(dest , "429 Too Many Requests\r\n");
		break;
	case 431:
		strcpy(dest , "431 Request Header Fields Too Large\r\n");
		break;
	case 444:
		strcpy(dest , "444 No Response\r\n");
		break;
	case 450:
		strcpy(dest , "450 Blocked by Windows Parental Controls\r\n");
		break;
	case 451:
		strcpy(dest , "451 Unavailable For Legal Reasons\r\n");
		break;
	case 494:
		strcpy(dest , "494 Request Header Too Large\r\n");
		break;
	case 500:
		strcpy(dest , "500 Internal Server Error\r\n");
		break;
	case 501:
		strcpy(dest , "501 Not Implemented\r\n");
		break;
	case 502:
		strcpy(dest , "502 Bad Gateway\r\n");
		break;
	case 503:
		strcpy(dest , "503 Service Unavailable\r\n");
		break;
	case 504:
		strcpy(dest , "504 Gateway Timeout\r\n");
		break;
	case 505:
		strcpy(dest , "505 HTTP Version Not Supported\r\n");
		break;
	case 506:
		strcpy(dest , "506 Variant Also Negotiates\r\n");
		break;
	case 507:
		strcpy(dest , "507 Insufficient Storage\r\n");
		break;
	case 508:
		strcpy(dest , "508 Loop Detected\r\n");
		break;
	case 510:
		strcpy(dest , "510 Not Extended\r\n");
		break;
	case 511:
		strcpy(dest , "511 Network Authentication Required\r\n");
		break;
	default:
		return -1;
	}
	return 1;
}

} // namespace http_handler