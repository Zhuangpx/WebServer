//  <z_http_conn.h>
//  HTTP连接 抽象封装


#ifndef _Z_HTTP_CONN_H
#define _Z_HTTP_CONN_H


#include <arpa/inet.h>  //  sockaddr_in

#include "../Utils/z_buffer.h"
#include "../Pool/z_sql_conn_RAII.h"
#include "z_http_request.h"
#include "z_http_response.h"
#include "../Log/z_log.h"


class zHttpConn
{
public:

  zHttpConn();
  ~zHttpConn();

  void init(int _sock_fd, const sockaddr_in& _addr); //  初始化
  void conn_close();                                 //  关闭连接

  int get_fd() const;           //  获取socket文件描述符
  int get_port() const;         //  获取客户端信息
  const char *get_ip() const;   //  获取客户端ip地址
  sockaddr_in get_addr() const; //  获取客户端端口

  ssize_t read(int *_error);  //  读取客户端发送的信息
  ssize_t write(int *_error); //  发送信息给客户端

  bool process();             //  解析和响应

  int to_write_bytes()
  { return iov_[0].iov_len + iov_[1].iov_len; }
  bool is_keep_alive()
  { return request_.is_keep_alive(); }

  static bool is_ET_;                 //  ET标记
  static const char* src_dir_;        //  根目录
  static std::atomic<int> user_count_;//  用户数量(连接数量)

private:

  int fd_;                    //  与客户端连接的socket文件描述符
  struct sockaddr_in addr_;   //  客户端连接的sockaddr

  bool close_fl_;             //  是否关闭

  int iov_cnt_;               //  iov向量计数
  struct iovec iov_[2];       //  iov

  zBuffer read_buff_;         //  读缓冲区
  zBuffer write_buff_;        //  写缓冲区

  zHttpRequest request_;      //  HTTP请求
  zHttpResponse response_;    //  HTTP响应

};


#endif  //  _Z_HTTP_CONN_H

/**
 * Stills & Src:
*/
