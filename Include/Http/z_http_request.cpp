#include "z_http_request.h"


// private: static const 保存所有html静态资源文件名
const std::set<std::string> zHttpRequest::DEFAULT_HTML_
{
  "/index",
  "/register",
  "/login",
  "/welcome",
  "/video",
  "/picture",
};
//  private: static const 保存并标记登录和注册
const std::map<std::string, int> zHttpRequest::DEFAULT_HTML_TAG_
{
    {"/register.html", 0},
    {"/login.html", 1},
};


/**
 * public:
*/

//  初始化请求报文
void zHttpRequest::init()
{
  method_ = path_ = version_ = body_ = "";
  state_ = REQUEST_LINE;
  header_.clear();
  post_.clear();
}

//  解析请求报文
bool zHttpRequest::parse(zBuffer& _buff)
{
  const char CRLF[] = "\r\n";
  if(_buff.readable_size() <= 0)
  { return false; }
  while(_buff.readable_size() > 0 && state_ != FINISH)
  {
    const char* _line_end = std::search(_buff.read_ptr(), _buff.write_ptr_const(), CRLF, CRLF+2);
    std::string _line(_buff.read_ptr(), _line_end);
    switch(state_)
    {
      case REQUEST_LINE:
        if(!parse_requset_line(_line))
        { return false; }
        parse_path();
        break;
      case HEADERS:
        parse_header(_line);
        if(_buff.readable_size() <= 2)
        { state_ = FINISH; }
        break;
      case BODY:
        parse_body(_line);
        break;
      default:
        break;
    }
    if(_line_end == _buff.write_ptr())
    { break; }
    _buff.set_read_ptr(_line_end + 2); //  后移两位 \r\n
  }
  LOG_DEBUG("HttpRequest parse: [%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
  return true;
}

//  API 获取文件资源路径 const
std::string zHttpRequest::path() const
{
  return path_;
}

//  API 获取文件资源路径 引用&
std::string& zHttpRequest::path()
{
  return path_;
}

//  API 获取请求方法
std::string zHttpRequest::method() const
{
  return method_;
}

//  API 获取http协议版本
std::string zHttpRequest::version() const
{
  return version_;
}

// Post method API 获取请求头部信息
std::string zHttpRequest::get_post(const std::string& _key) const
{
  assert(_key != "");
  if(post_.count(_key) == 1)
  { return post_.find(_key)->second; }
  return "";
}
std::string zHttpRequest::get_post(const char* _key) const
{
  assert(_key != nullptr);
  if(post_.count(_key) == 1)
  { return post_.find(_key)->second; }
  return "";
}

// 判断链接状态 (是否keep-alive)
bool zHttpRequest::is_keep_alive() const
{
  if(header_.count("Connection") == 1)
  { return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";}
  return false;
}

/**
 * private:
*/

//  解析请求行
bool zHttpRequest::parse_requset_line(const std::string &_line)
{
  std::regex _patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    /*
    ^([^ ]*) ([^ ]*) HTTP/([^ ]*)$ 匹配HTTP请求行：
      - ^ 匹配字符串的开头
      - ([^ ]*) 匹配任何非空格字符零次或多次，且保存匹配结果到一个分组中
      - 空格字符用来匹配HTTP请求行中不同部分之间的空格
      - HTTP/ 匹配字符串"HTTP/"，注意这里不保存匹配结果到分组中
      - ([^ ]*) 再次匹配任何非空格字符零次或多次，保存匹配结果到另一个分组中
      - $ 匹配字符串的结尾
    整个正则表达式的意义是匹配由三部分组成的HTTP请求行，每个部分之间由一个空格分隔，分别为：
      - 第一部分是任何非空格字符零次或多次，保存匹配结果到第一个分组中
      - 第二部分同样是任何非空格字符零次或多次，保存匹配结果到第二个分组中
      - 第三部分是以"HTTP/"开头的字符串，不保存匹配结果到任何分组中，紧随其后的是任何非空格字符零次或多次，保存匹配结果到第三个分组中
    例子：
    GET /index.html HTTP/1.1
    结果：
    其中第一个分组匹配"GET"，第二个分组匹配"/index.html"，第三个分组匹配"1.1"。
    */
  std::smatch _match_res;
  bool _match_fl = std::regex_match(_line, _match_res, _patten);
  if(_match_fl == false)
  {
    LOG_ERROR("Http RequestLine Error!");
    return false;
  }
  else
  {
    //  _match_res[0] == _line
    method_ = _match_res[1];  //  请求方法
    path_ = _match_res[2];    //  请求URL路径
    version_ = _match_res[3]; //  协议版本
    state_ = HEADERS;         //  请求行解析完 主状态机 转移到 HEADERS (请求头)
    return true;
  }
  return true;  //  default return_value
}

//  解析请求头
void zHttpRequest::parse_header(const std::string &_line)
{
  std::regex _patten("^([^:]*): ?(.*)$");
    /*
    ^([^:]*): ?(.*)$ 匹配HTTP请求头：
      - "^"表示匹配字符串的开头。
      - "([^:])"表示匹配任意非冒号字符（""表示零个或多个）。
      - ":"表示匹配一个冒号。
      - " ?"表示匹配零个或一个空格，其中"?"表示可选项。
      - "(.)"表示匹配任意字符（包括空格）（""表示零个或多个）。
      - "$"表示匹配字符串的结尾。
    例如"Content-Length: 12345"，可以将"Content-Length"和"12345"分别匹配出来。
    */
  std::smatch _match_res;
  bool _match_fl = std::regex_match(_line, _match_res, _patten);
  if(_match_fl == false)
  {
    state_ = BODY;  //  请求头解析完 主状态机 转移到 BODY (请求数据/体)
  }
  else
  {
    header_[_match_res[1]] = _match_res[2];
  }
}

//  解析请求数据(请求体)
void zHttpRequest::parse_body(const std::string &_line)
{
  body_ = _line;
  parse_post();     //  有请求数据/体 说明是post请求
  state_ = FINISH;  //  请求头解析完 主状态机 转移到 FINISH (结束)
  LOG_DEBUG("Http Request: Body:%s, len:%d", body_.c_str(), body_.size());
}

//  解析URL资源路径
void zHttpRequest::parse_path()
{
  if(path_ == "/")
  {
    path_ = "/index.html";  //  默认index
  }
  else
  {
    for(auto &iter:DEFAULT_HTML_)
    {
      if(iter == path_)
      {
        path_ += ".html";
        break;
      }
    }
  }
}

//  post方式时 解析请求数据/体 此处请求体携带登录/注册信息
void zHttpRequest::parse_post()
{
  if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded")
  {
    parse_body_post_form_urlencoded_();
    int _cnt = DEFAULT_HTML_TAG_.count(path_);
    if(_cnt > 0)
    {
      auto _tag = DEFAULT_HTML_TAG_.find(path_)->second;
      LOG_DEBUG("Http Request With Post Form Urlencoded: Tag: %d", _tag);
      if(_tag == 0 || _tag == 1)
      {
        bool _is_login = (_tag == 1);
        bool _user_verify_res = user_verify(post_["username"], post_["password"], _is_login);
        if(_user_verify_res == true)
        {
          path_ = "/welcome.html";
        }
        else
        {
          path_ = "/error.html";
        }
      }
    }
  }
}


//  当通过 HTTP POST 方法请求且 form 的 enctype 属性被设置为 "application/x-www-form-urlencoded" 时 针对这种格式解析数据
//  保存 name-pwd
void zHttpRequest::parse_body_post_form_urlencoded_()
{
  //  [key=value]&[key=value]... 形式
  if(body_.size() == 0)
  { return ; }
  std::string _key, _value;
  int _num = 0, _size = body_.size();
  int _i = 0, _j = 0;     //  i:now j:pre
  for(; _i < _size; _i++)
  {
    char _ch = body_[_i];
    switch (_ch)
    {
      case '=':           //  [key] = [value] key 结束后是 =
        _key = body_.substr(_j, _i-_j);
        _j = _i+1;
        break;
      case '+':           //  空格会被替换成+ 换回来
        body_[_i] = ' ';
        break;
      case '%':           //  % 后接两位的十六进制数
        _num = conver_hex(body_[_i+1])*16 + conver_hex(body_[_i+2]);
        body_[_i+1] = _num/10 + '0';
        body_[_i+2] = _num%10 + '0';
        _i += 2;
        break;
      case '&':           //  [key] = [value] value 结束后是&
        _value = body_.substr(_j, _i-_j);
        _j = _i + 1;
        post_[_key] = _value;
        LOG_DEBUG("Http Request [key=value] : %s = %s", _key.c_str(), _value.c_str());
        break;
      default:
        break;
    }
  }
  assert(_j <= _i);
  //  如果是在末尾 没有&来连接下一个 特判一下
  if(post_.count(_key) == 0 && _j < _i)
  {
    _value = body_.substr(_j, _i - _j);
    post_[_key] = _value;
  }
}


//  用户登录注册确认处理 (static)
//  _is_login 1:登录 0:注册
bool zHttpRequest::user_verify(const std::string& _name, const std::string& _pwd, bool _is_login)
{
  if(_name == "" || _pwd == "")
  { return false; }

  LOG_INFO("Http Request: User Verify: name:%s password:%s", _name.c_str(), _pwd.c_str());

  MYSQL* _sql;
  zSqlConnRAII(&_sql, zSqlConnPool::instance());
  assert(_sql); //  !=nullptr
  bool _fl = false;
  char _order[256] = { 0 };
  MYSQL_RES* _res = nullptr;

  if(!_is_login)
  { _fl = true; }

  //  SQL: 查询用户名和密码
  //  SELECT username, password FROM user WHERE username='%s' LIMIT 1
  snprintf(_order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", _name.c_str());
  LOG_DEBUG("Http Request: SQL_SEL: %s", _order);

  if(mysql_query(_sql, _order))
  {
    mysql_free_result(_res);
    return false;
  }

  _res = mysql_store_result(_sql);
  /*unsigned int _j = */      mysql_num_fields(_res);
  /*MYSQL_FIELD* _fields = */ mysql_fetch_fields(_res);

  while(MYSQL_ROW _row = mysql_fetch_row(_res))
  {
    LOG_DEBUG("Http Request: SQL_search_result: %s %s", _row[0], _row[1]);
    std::string _password(_row[1]);
    if(_is_login)
    {
      if(_pwd == _password)
      { _fl = true; }
      else
      {
        //  登录 但不正确
        _fl = false;
        LOG_DEBUG("Http Request: SQL_user_verify: password error!");
      }
    }
    else
    {
      //  注册 但已存在
      _fl = false;
      LOG_DEBUG("Http Request: SQL_user_verify: user used!");
    }
  }
  mysql_free_result(_res);

  //  注册 且 用户名未被使用
  if(!_is_login && _fl)
  {
    LOG_DEBUG("Http Request: SQL_user_verify: regirster!");
    bzero(_order, 256);
    snprintf(_order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", _name.c_str(), _pwd.c_str());
    LOG_DEBUG("Http Request: SQL_regirster: %s", _order);
    if(mysql_query(_sql, _order))
    {
      LOG_DEBUG("Http Request: SQL_regirster: SQL insert error!");
      _fl = false;
    }
    _fl = true;
  }
  zSqlConnPool::instance() -> free_conn(_sql);
  LOG_DEBUG("Http Request: SQL_user_verify: success!");
  return _fl;
}


//  十六进制char 转十进制int
int zHttpRequest::conver_hex(char _ch)
{
  if(_ch >= 'A' && _ch <='F') return (_ch-'A')+10;
  if(_ch >= 'a' && _ch <='f') return (_ch-'a')+10;
  return _ch;
}

