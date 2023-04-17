#include "z_http_response.h"

/**
 * public:
*/

//  构造函数
zHttpResponse::zHttpResponse()
{
  code_ = -1;
  path_ = src_dir_ = "";
  is_keep_alive_ = false;
  mm_file_ = nullptr;
  mm_file_stat_ = { 0 };
}

//  析构函数
zHttpResponse::~zHttpResponse()
{
  unmap_file();
}

//  初始化响应报文
void zHttpResponse::init(const std::string& _src_dir, std::string& _path, bool _is_keep_alive, int _code)
{
  assert(_src_dir != "");
  if(mm_file_)
  { unmap_file(); }
  code_ = _code;
  is_keep_alive_ = _is_keep_alive;
  path_ = _path;
  src_dir_ = _src_dir;
  mm_file_ = nullptr;
  mm_file_stat_ = { 0 };
}

//  构造响应报文
void zHttpResponse::make_response(zBuffer& _buff)
{
  if(stat((src_dir_+path_).data(), &mm_file_stat_) < 0 || S_ISDIR(mm_file_stat_.st_mode))
  { code_ = 404; }
  else if(!(mm_file_stat_.st_mode & S_IROTH))
  { code_ = 403; }
  else if(code_ == -1)
  { code_ = 200; }

  error_html();
  add_state_line(_buff);
  add_header(_buff);
  add_content(_buff);
}

//  关闭文件共享区
void zHttpResponse::unmap_file()
{
  if(mm_file_)
  {
    munmap(mm_file_, mm_file_stat_.st_size);
    mm_file_ = nullptr;
  }
}

//  处理获取响应正文失败
void zHttpResponse::error_content(zBuffer &_buff, std::string _message)
{
  std::string _body = (std::string)"<html><title>Error</title>" + (std::string)"<body bgcolor=\"ffffff\">";
  std::string _status;
  if(CODE_STATUS.count(code_) == 1)
  { _status = CODE_STATUS.find(code_)->second; }
  else
  { _status = "Bad Request"; }
  _body += std::to_string(code_) + " : " + _status + "\n";
  _body += "<p>" + _message + "</p>";
  _body += "<hr><em>Zpx_WebServer</em></body></html>";
  _buff.push("Content-length: " + std::to_string(_body.size()) + "\r\n\r\n");
  _buff.push(_body);
}


/**
 * private:
*/


// 后缀名 - 文件类型
const std::map<std::string, std::string> zHttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

// 状态码 - 状态
const std::map<int, std::string> zHttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

// 状态码 - 页面路径
const std::map<int, std::string> zHttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};


//  构造响应报文状态行
void zHttpResponse::add_state_line(zBuffer& _buff)
{
  std::string _status;
  if(CODE_STATUS.count(code_) == 1)
  { _status = CODE_STATUS.find(code_)->second; }
  else
  {
    code_ = 400;
    _status = CODE_STATUS.find(code_)->second;
  }
  _buff.push("HTTP/1.1 " + std::to_string(code_) + " " + _status + "\r\n");
}


//  构造响应报文消息报头
void zHttpResponse::add_header(zBuffer& _buff)
{
  _buff.push("Connection: ");
  if(is_keep_alive_)
  {
    _buff.push("keep-alive\r\n");
    _buff.push("keep-alive: max=6, timeout=120\r\n");
  }
  else
  {
    _buff.push("close\r\n");
  }
  _buff.push("Content-type: " + file_type() + "\r\n");
}

//  构造响应报文响应正文
void zHttpResponse::add_content(zBuffer& _buff)
{
  int _src_fd = open((src_dir_+path_).data(), O_RDONLY);
  if(_src_fd < 0)
  {
    error_content(_buff, "File NotFound!");
    return ;
  }
  //  将文件内存映射
  LOG_DEBUG("Http Response: file path: %s", (src_dir_+path_).data());
  int* _mm_file_ptr = (int*)mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, _src_fd, 0);
  if(*_mm_file_ptr == -1)
  {
    error_content(_buff, "File NotFound!");
    return ;
  }
  mm_file_ = (char*)_mm_file_ptr;
  close(_src_fd);
  _buff.push("Content-length: " + std::to_string(mm_file_stat_.st_size) + "\r\n\r\n");
}

//  错误状态码处理页面
void zHttpResponse::error_html()
{
  if(CODE_PATH.count(code_) == 1)
  {
    path_ = CODE_PATH.find(code_)->second;
    stat((src_dir_+path_).data(), &mm_file_stat_);
  }
}


//  获取资源文件类型
std::string zHttpResponse::file_type()
{
  std::string::size_type _idx = path_.find_last_of('.'); //  找后缀
  if(_idx == std::string::npos)
  { return "text/plain"; }
  std::string _suffix = path_.substr(_idx);
  if(SUFFIX_TYPE.count(_suffix) == 1)
  { return SUFFIX_TYPE.find(_suffix)->second; }
  return "text/plain";
}


