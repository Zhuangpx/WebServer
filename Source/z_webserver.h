#ifndef _Z_WEBSERVER_H
#define _Z_WEBSERVER_H

#include <memory>   //  std::unique_ptr
#include <map>

#include "../Include/Utils/z_epoller.h"
#include "../Include/Utils/z_timer.h"
#include "../Include/Log/z_log.h"
#include "../Include/Pool/z_sql_conn_pool.h"
#include "../Include/Pool/z_sql_conn_RAII.h"
#include "../Include/Pool/z_threadpool.h"
#include "../Include/Http/z_http_conn.h"

class zWebServer
{
public:

  zWebServer() = default;
  zWebServer(
            int _port, int _mode, int _time_out_MS, bool _opt_linger,
            int _sql_port, const char* _sql_user, const char* _sql_pwd,
            const char* _db_name, int _conn_pool_num, int _thread_num,
            bool _open_log, int _log_level, int _log_que_size
            );
  ~zWebServer();
  void server();  //  启动server服务

private:

  bool init_socket();                          //  套接字初始化
  void init_event_mode(int _new_mode);         //  初始化事件模式
  void add_client(int _fd, sockaddr_in _addr); //  添加客户端

  void deal_listen();                  //  处理套接字监听事件
  void deal_write(zHttpConn *_client); //  处理客户端写事件
  void deal_read(zHttpConn *_client);  //  处理客户端读事件

  void send_error(int _fd, const char *_info); //  发送错误信息
  void extent_time(zHttpConn *_client);        //  更新客户端超时时间
  void close_conn(zHttpConn *_client);         //  关闭连接

  void on_read(zHttpConn *_client);    //  用于处理连接的读事件
  void on_write(zHttpConn *_client);   //  用于处理连接的写事件
  void on_process(zHttpConn *_client); //  处理连接

  static const int MAX_FD = 65536;      //  最大fd计数
  static int set_fd_non_block(int _fd); //  将监听套接字设置为非阻塞模式

  int port_;         //  开放端口
  bool open_linger_; //  是否开启linger
  int time_out_MS_;  //  超时时间(可能没有)
  bool is_close_;    //  是否关闭
  int listen_fd_;    //  用于监听的文件描述符
  char *src_dir_;    //  资源根目录

  uint32_t listen_event_; //  监听事件模式
  uint32_t conn_event_;   //  连接事件模式

  std::unique_ptr<zTimer> timer_;           //  Timer
  std::unique_ptr<zThreadPool> threadpool_; //  ThreadPool
  std::unique_ptr<zEpoller> epoller_;       //  epoller
  std::map<int, zHttpConn> users_;          //  每个用户有个连接映射

};

#endif  //  _Z_WEBSERVER_H

/**
 * Stills & Src:
*/
