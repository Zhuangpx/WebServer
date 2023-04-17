#include "z_webserver.h"

/**
 * public:
*/


zWebServer::zWebServer(
                      int _port, int _mode, int _time_out_MS, bool _opt_linger,
                      int _sql_port, const char* _sql_user, const char* _sql_pwd,
                      const char* _db_name, int _conn_pool_num, int _thread_num,
                      bool _open_log, int _log_level, int _log_que_size
                      ):
                      port_(_port), open_linger_(_opt_linger), time_out_MS_(_time_out_MS), is_close_(false),
                      timer_(new zTimer()), threadpool_(new zThreadPool(_thread_num)), epoller_(new zEpoller())
{
  /**
   * _port: 服务器开放端口号
   * _mode: 事件模式
   * _time_out_MS: 超时时间
   * _opt_linger: 是否开启linger模式
   * _sql_port sql_user sql_pwd: 数据库端口 用户 密码
   * _db_name: 数据库名
   * _conn_pool_num: 数据库连接池数量
   * _thread_num: 线程池数量
   * _open_log: 是否开启日志
   * _log_level: 日志等级
   * _log_que_size: 日志队列长度
  */
  //  src_dir_ zHttpConn zSqlConnPoll
  src_dir_ = getcwd(nullptr, 256);
  assert(src_dir_);
  strncat(src_dir_, "/Resource/", 16);
  zHttpConn::user_count_ = 0;
  zHttpConn::src_dir_ = src_dir_;
  zSqlConnPool::instance()->init("localhost", _sql_port, _sql_user, _sql_pwd, _db_name, _conn_pool_num);
  //  event_mode socket
  init_event_mode(_mode);
  if(!init_socket())
  { is_close_ = true; }
  //  log
  if(_open_log)
  {
    zLog::instance()->init(_log_level, "./log", ".log", _log_que_size);
    if(is_close_)
    {
      LOG_ERROR("========== Server Init Error!==========");
    }
    else
    {


      LOG_INFO(" +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ ");
      LOG_INFO(" |Z|h|u|a|n|g|p|x|-|W|e|b|S|e|r|v|e|r| ");
      LOG_INFO(" +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ ");
      LOG_INFO("========== Server Init ==========");
      LOG_INFO("Port:%d, OpenLinger: %s", port_, _opt_linger? "true":"false");
      LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                      (listen_event_ & EPOLLET ? "ET": "LT"),
                      (conn_event_ & EPOLLET ? "ET": "LT"));
      LOG_INFO("LogSys level: %d", _log_level);
      LOG_INFO("SrcDir: %s", zHttpConn::src_dir_);
      LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", _conn_pool_num, _thread_num);
    }
  }
}

zWebServer::~zWebServer()
{
  close(listen_fd_);
  is_close_ = true;
  free(src_dir_);
  zSqlConnPool::instance()->close_pool();
}


void zWebServer::server()
{
  //  epoll wait timeout == -1 无事件将阻塞
  int _time_MS = -1;
  if(!is_close_)
  {
    LOG_INFO("========== Server Start ==========");
  }
  //  轮询
  while(!is_close_)
  {
    //  如果设计了超时时间
    if(time_out_MS_ > 0)
    { _time_MS = timer_->next_tick(); }
    int _event_cnt = epoller_->wait(_time_MS);
    for(int _i = 0; _i < _event_cnt; ++_i)
    {
      int _fd = epoller_->get_eventfd(_i);
      uint32_t _events = epoller_->get_events(_i);
      //  监听
      if(_fd == listen_fd_)
      {
        deal_listen();
      }
      //  连接断开,HUP或ERR事件
      else if(_events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
      {
        assert(users_.count(_fd) > 0);
        close_conn(&users_[_fd]);
      }
      //  可读事件
      else if(_events & EPOLLIN)
      {
        assert(users_.count(_fd) > 0);
        deal_read(&users_[_fd]);
      }
      //  可写事件
      else if(_events & EPOLLOUT)
      {
        assert(users_.count(_fd) > 0);
        deal_write(&users_[_fd]);
      }
      //  其它情况为异常
      else
      {
        LOG_ERROR("Server Error: Unexpected event!");
      }
    }
  }
}

/**
 * private:
*/

//  套接字初始化
bool zWebServer::init_socket()
{
  int _res;
  struct sockaddr_in _addr;
  struct linger _opt_linger = { 0 };
  //  检查端口是否合法
  if(port_ > 65535 || port_ < 1024)
  {
    LOG_ERROR("Port:%d error!",  port_);
    return false;
  }
  //  预处理套接字属性
  _addr.sin_family = AF_INET;
  _addr.sin_addr.s_addr = htonl(INADDR_ANY);
  _addr.sin_port = htons(port_);
  //  预处理linger模式 优雅关闭: 直到所剩数据发送完毕或超时
  if(open_linger_)
  {
    _opt_linger.l_onoff = 1;
    _opt_linger.l_linger = 1;
  }
  //  创建监听套接字
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_fd_ < 0)
  {
    LOG_ERROR("Create listen-socket error!", port_);
    return false;
  }
  //  设置套接字linger模式
  _res = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &_opt_linger, sizeof(_opt_linger));
  if(_res < 0)
  {
    LOG_ERROR("Init linger error!", port_);
    close(listen_fd_);
    return false;
  }

  int optval = 1;
  /* 端口复用 */
  /* 只有最后一个套接字会正常接收数据。 */
  //  设置套接字SO_REUSEADDR属性 运行端口复用
  _res = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
  if(_res == -1) {
      LOG_ERROR("Set socket setsockopt error!");
      close(listen_fd_);
      return false;
  }
  //  绑定套接字到本地地址 并设置属性(监听)
  _res = bind(listen_fd_, (struct sockaddr *)&_addr, sizeof(_addr));
  if(_res < 0) {
      LOG_ERROR("Bind Port:%d error!", port_);
      close(listen_fd_);
      return false;
  }
  //  设置监听
  _res = listen(listen_fd_, 6);
  if(_res < 0) {
      LOG_ERROR("Listen port:%d error!", port_);
      close(listen_fd_);
      return false;
  }
  //  将监听套接字添加到epoll事件轮询器中，同时设置其监听事件类型为EPOLLIN，表示等待有客户端连接请求
  _res = epoller_->add_fd(listen_fd_,  listen_event_ | EPOLLIN);
  if(_res == 0) {
      LOG_ERROR("Add listen error!");
      close(listen_fd_);
      return false;
  }
  //  将监听套接字设置为非阻塞模式
  set_fd_non_block(listen_fd_);
  LOG_INFO("Server port:%d", port_);
  return true;
}


//  初始化事件模式
void zWebServer::init_event_mode(int _new_mode)
{
  //  0-(LT)水平触发  1-(ET)边缘触发
  listen_event_ = EPOLLRDHUP;
  conn_event_ = EPOLLONESHOT | EPOLLRDHUP;
  switch(_new_mode) //  [][]
  {
    case 0: //  [0][0]
      break;
    case 1: //  [0][1]
      conn_event_ |= EPOLLET;
      break;
    case 2: //  [1][0]
      listen_event_ |= EPOLLET;
      break;
    case 3: //  [1][1]
      listen_event_ |= EPOLLET;
      conn_event_ |= EPOLLET;
      break;
    default://  [1][1]
      listen_event_ |= EPOLLET;
      conn_event_ |= EPOLLET;
      break;
  }
  zHttpConn::is_ET_ = (conn_event_ & EPOLLET);
}


//  添加客户端
void zWebServer::add_client(int _fd, sockaddr_in _addr)
{
  assert(_fd > 0);
  users_[_fd].init(_fd, _addr);
  if(time_out_MS_ > 0)
  {
    timer_->add(_fd, time_out_MS_, std::bind(&zWebServer::close_conn, this, &users_[_fd]));
  }
  epoller_->add_fd(_fd, EPOLLIN | conn_event_);
  set_fd_non_block(_fd);
  LOG_INFO("Client[%d] in!", users_[_fd].get_fd());
}


//  处理套接字监听事件
void zWebServer::deal_listen()
{
  struct sockaddr_in _addr;
  socklen_t _len = sizeof(_addr);
  do
  {
    int _fd = accept(listen_fd_, (struct sockaddr*)&_addr, &_len);
    if(_fd <= 0)
    { return ; }
    else if(zHttpConn::user_count_ >= MAX_FD)
    {
      send_error(_fd, "Server busy!");
      LOG_WARN("Listen error: clients is full!");
      return ;
    }
    add_client(_fd, _addr);
  } while(listen_event_ & EPOLLET);
}


//  处理客户端写事件
void zWebServer::deal_read(zHttpConn* _client)
{
  assert(_client);
  extent_time(_client);
  threadpool_->add_task(std::bind(&zWebServer::on_read, this, _client));
}


//  处理客户端读事件
void zWebServer::deal_write(zHttpConn* _client)
{
  assert(_client);
  extent_time(_client);
  threadpool_->add_task(std::bind(&zWebServer::on_write, this, _client));

}

//  发送错误信息
void zWebServer::send_error(int _fd, const char* _info)
{
  assert(_fd > 0);
  int _res = send(_fd, _info, strlen(_info), 0);
  if(_res < 0)
  {
    LOG_WARN("Send error to client[%d] error!", _fd);
  }
  close(_fd);
}

//  更新客户端超时时间
void zWebServer::extent_time(zHttpConn* _client)
{
  assert(_client);
  if(time_out_MS_ > 0)
  { timer_->adjust(_client->get_fd(), time_out_MS_); }
}

//  关闭连接
void zWebServer::close_conn(zHttpConn* _client)
{
  assert(_client);
  LOG_INFO("Client[%d] quit!", _client->get_fd());
  epoller_->delete_fd(_client->get_fd());
  _client->conn_close();
}

//  用于处理连接的读事件
void zWebServer::on_read(zHttpConn* _client)
{
  assert(_client);
  int _res = -1, _read_error = 0;
  _res = _client->read(&_read_error);
  if(_res <= 0 && _read_error != EAGAIN)
  {
    close_conn(_client);
    return ;
  }
  on_process(_client);
}

//  用于处理连接的写事件
void zWebServer::on_write(zHttpConn* _client)
{
  assert(_client);
  int _res = -1, _write_errno = 0;
  _res = _client->write(&_write_errno);
  if(_client->to_write_bytes() == 0)
  {
    /* 传输完成 */
    if(_client->is_keep_alive())
    {
      on_process(_client);
      return ;
    }
  }
  else if(_res < 0)
  {
    if(_write_errno == EAGAIN)
    {
      /* 继续传输 */
      epoller_->modify_fd(_client->get_fd(), conn_event_ | EPOLLOUT);
      return ;
    }
  }
  close_conn(_client);
}

//  处理连接
void zWebServer::on_process(zHttpConn* _client)
{
  if(_client->process())
  {
    epoller_->modify_fd(_client->get_fd(), conn_event_ | EPOLLOUT);
  }
  else
  {
    epoller_->modify_fd(_client->get_fd(), conn_event_ | EPOLLIN);
  }
}

int zWebServer::set_fd_non_block(int fd)
{
  assert(fd > 0);
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}