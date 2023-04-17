//  <z_http_response.h>
//  http响应 抽象封装

#ifndef _Z_HTTP_RESPONSE_H
#define _Z_HTTP_RESPONSE_H

#include <string>
#include <map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../Utils/z_buffer.h"
#include "../Log/z_log.h"

class zHttpResponse
{
public:
  zHttpResponse();
  ~zHttpResponse();

  //  初始化响应报文
  void init(const std::string& _src_dir, std::string& _path, bool _is_keep_alive = false, int _code = -1);

  void make_response(zBuffer &_buff);                        //  构造响应报文
  void unmap_file();                                         //  关闭文件共享区
  char *map_file() { return mm_file_; }                      //  获取内存映射的地址
  size_t file_size() const { return mm_file_stat_.st_size; } //  获取文件大小
  int code() const { return code_; }                         //  获取状态码
  void error_content(zBuffer &_buff, std::string _message);  //  处理获取响应正文失败

private:

  void add_state_line(zBuffer &_buff); //  构造响应报文状态行
  void add_header(zBuffer &_buff);     //  构造响应报文消息报头
  void add_content(zBuffer &_buff);    //  构造响应报文响应正文

  void error_html();       //  错误状态码处理页面
  std::string file_type(); //  获取资源文件类型

  int code_;           //  状态码
  bool is_keep_alive_; //  is-keep-alive

  std::string path_;    //  资源路径
  std::string src_dir_; //  资源根路径/目录

  char *mm_file_;            //  内存映射 - 文件
  struct stat mm_file_stat_; //  内存映射 - stat

  // 后缀名 - 文件类型
  static const std::map<std::string, std::string> SUFFIX_TYPE;
  // 状态码 - 状态
  static const std::map<int, std::string> CODE_STATUS;
  // 状态码 - 页面路径
  static const std::map<int, std::string> CODE_PATH;

};


#endif  //  _Z_HTTP_RESPONSE_H

/**
 * Stills & Src:
 * 内存映射
*/

