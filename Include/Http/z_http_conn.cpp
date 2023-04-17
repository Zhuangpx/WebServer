#include "z_http_conn.h"


/**
 * public:
*/

//  static
const char* zHttpConn::src_dir_;
std::atomic<int> zHttpConn::user_count_;
bool zHttpConn::is_ET_;

zHttpConn::zHttpConn()
{
  fd_ = -1;
  addr_ = { 0 };
  close_fl_ = true;
}

zHttpConn::~zHttpConn()
{
  conn_close();
}

//  初始化
void zHttpConn::init(int _fd, const sockaddr_in& _addr)
{
  assert(_fd > 0);
  user_count_++;
  addr_ = _addr;
  fd_ = _fd;
  write_buff_.clear_buffer();
  read_buff_.clear_buffer();
  close_fl_ = false;
  LOG_INFO("Http Conn: Client[%d](%s:%d) in, user_count:%d", fd_, get_ip(), get_port(), (int)user_count_);
}

//  关闭连接
void zHttpConn::conn_close()
{
  response_.unmap_file();
  if(!close_fl_)
  {
    close_fl_ = true;
    user_count_--;
    close(fd_);
    LOG_INFO("Http Conn: Client[%d](%s:%d) quit, user_count:%d", fd_, get_ip(), get_port(), (int)user_count_);
  }
}

//  获取socket文件描述符
int zHttpConn::get_fd() const
{ return fd_; }

//  获取客户端信息
struct sockaddr_in zHttpConn::get_addr() const
{ return addr_; }

//  获取客户端ip地址
const char* zHttpConn::get_ip() const
{ return inet_ntoa(addr_.sin_addr); }

//  获取客户端端口
int zHttpConn::get_port() const
{ return addr_.sin_port; }


//  读取客户端发送的信息
ssize_t zHttpConn::read(int* _error)
{
  ssize_t _size = -1;
  do
  {
    _size = read_buff_.read_fd(fd_, _error);
    if (_size <= 0)
    { break; }
  } while(is_ET_);
  return _size;
}

//  发送信息给客户端
ssize_t zHttpConn::write(int* _error)
{
  ssize_t _size = -1;
  do
  {
    _size = writev(fd_, iov_, iov_cnt_);
    if(_size <= 0)  //  写入失败
    {
      *_error = errno;
      break;
    }
    if(iov_[0].iov_len + iov_[1].iov_len == 0)  //  全部写入 传输结束
    { break; }
    else if(static_cast<size_t>(_size) > iov_[0].iov_len) //  iov[0]发送完
    {
      //  size = all_iov[0].len + now_iov[1].len
      iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (_size - iov_[0].iov_len);
      iov_[1].iov_len -= (_size - iov_[0].iov_len);
      if(iov_[0].iov_len)   //  清空iov[0]
      {
        write_buff_.clear_buffer();
        iov_[0].iov_len = 0;
      }
    }
    else  //  iov[0]还没发送完
    {
      iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + _size;
      iov_[0].iov_len -= _size;
      write_buff_.update_read_ptr(_size);
    }
  } while(is_ET_ || to_write_bytes() > 10240);
  return _size;
}

//  解析和响应
bool zHttpConn::process()
{
  //  解析请求报文 并初始化响应报文
  request_.init();
  if(read_buff_.readable_size() <= 0)
  { return false; }
  else if(request_.parse(read_buff_))
  {
    LOG_DEBUG("Http Conn: %s", request_.path().c_str());
    response_.init(src_dir_, request_.path(), request_.is_keep_alive(), 200);
  }
  else
  {
    response_.init(src_dir_, request_.path(), false, 400);
  }
  //  响应报文
  //  iov_[0]:响应头 iov_[1]:文件
  response_.make_response(write_buff_);
  iov_[0].iov_base = const_cast<char*>(write_buff_.read_ptr());
  iov_[0].iov_len = write_buff_.readable_size();
  iov_cnt_ = 1;
  if(response_.file_size() > 0 && response_.map_file())
  {
    iov_[1].iov_base = response_.map_file();
    iov_[1].iov_len = response_.file_size();
    iov_cnt_ = 2;
  }
  LOG_DEBUG("Http Conn: filesize:%d, %d  to %d", response_.file_size(), iov_cnt_, to_write_bytes());
  return true;
}

/**
 * private:
*/

