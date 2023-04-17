//  <z_http_request.h>
//  http请求 抽象封装

#ifndef _Z_HTTP_REQUEST_H
#define _Z_HTTP_REQUEST_H


#include <map>
#include <set>
#include <string>
#include <regex> //  正则表达式
#include <errno.h>
#include <mysql/mysql.h>


#include "../Utils/z_buffer.h"
#include "../Pool/z_sql_conn_pool.h"
#include "../Log/z_log.h"
#include "../Pool/z_sql_conn_RAII.h"


class zHttpRequest
{
public:

  //  主状态机状态
  enum PARSE_STATE
  {
    REQUEST_LINE = 0,
    HEADERS,
    BODY,
    FINISH,
  };

  //  报文解析结果
  enum HTTP_CODE
  {
    NO_REQUEST = 0,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURSE,
    FORBIDDENT_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION,
  };

  zHttpRequest()
  { init(); }
  ~zHttpRequest() = default;

  void init();                //  初始化请求报文
  bool parse(zBuffer& _buff); //  解析请求报文

  //  API
  std::string path() const;    //  API 获取文件资源路径 const
  std::string& path();         //  API 获取文件资源路径 引用&
  std::string method() const;  //  API 获取请求方法
  std::string version() const; //  API 获取http协议版本
  //  Post method API 获取请求头部信息
  std::string get_post(const std::string& _key) const;
  std::string get_post(const char* _key) const;

  bool is_keep_alive() const;  // 判断链接状态 (是否keep-alive)

private:
  bool parse_requset_line(const std::string &_line); //  解析请求行
  void parse_header(const std::string &_line);       //  解析请求头
  void parse_body(const std::string &_line);         //  解析请求体

  void parse_path();             //  解析URL资源路径
  void parse_post();             //  post方式时 解析请求数据/体 此处请求体携带登录/注册信息
  //  当通过HTTP POST方法请求且form的enctype属性被设置为"application/x-www-form-urlencoded"时 针对这种格式解析数据
  //  保存 name-pwd
  void parse_body_post_form_urlencoded_();
  //  用户登录注册确认处理
  static bool user_verify(const std::string& _name, const std::string& _pwd, bool _is_login);

  //  报文解析状态
  PARSE_STATE state_;

  //  报文信息
  std::string method_;
  std::string path_;
  std::string version_;
  std::string body_;

  //  报文头部 类型-数据
  std::map<std::string, std::string> header_;

  //  报文数据 类型-数据
  std::map<std::string, std::string> post_;

  //  静态常量 保存所有html静态资源文件名
  static const std::set<std::string> DEFAULT_HTML_;

  //  静态常量 标记登录和注册
  static const std::map<std::string, int> DEFAULT_HTML_TAG_;

  //  十六进制char 转十进制int
  static int conver_hex(char _ch);


};


#endif  //  _Z_HTTP_REQUEST_H

/**
 * Stills & Src:
 * mutex & condition_variable
 * shared_ptr
 * 通用引用
*/
