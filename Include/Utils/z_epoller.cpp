#include "z_epoller.h"

/**
 * public:
*/

//  构造函数 初始化监听事件列表 创建epoll连接
zEpoller::zEpoller(int max_event) : epoll_fd_(epoll_create(1024)), events_(max_event)
{
  assert(epoll_fd_ >= 0 && events_.size() > 0);
}

//  析构函数 关闭epoll连接
zEpoller::~zEpoller()
{
  close(epoll_fd_);
}

//  添加文件监听
bool zEpoller::add_fd(int _fd, uint32_t _events)
{
  if(_fd < 0) return false;
  epoll_event _evt = {0};
  _evt.events = _events;
  _evt.data.fd = _fd;
  return (0 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, _fd, &_evt));
}

//  修改文件监听属性
bool zEpoller::modify_fd(int _fd, uint32_t _events)
{
  if(_fd < 0) return false;
  epoll_event _evt = {0};
  _evt.events = _events;
  _evt.data.fd = _fd;
  return (0 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, _fd, &_evt));
}

//  删除文件监听
bool zEpoller::delete_fd(int _fd)
{
  if(_fd < 0) return false;
  epoll_event _evt = {0};
  return (0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, _fd, &_evt));
}

//  等待监听事件
int zEpoller::wait(int _timeout/*=-1*/)
{
  return epoll_wait(epoll_fd_, &events_[0], static_cast<int>(events_.size()), _timeout);
}

//  获取事件文件描述符
int zEpoller::get_eventfd(size_t _idx) const
{
  assert(_idx >=0 && _idx < events_.size());
  return events_[_idx].data.fd;
}

//  获取监听事件
uint32_t zEpoller::get_events(size_t _idx) const
{
  assert(_idx >=0 && _idx < events_.size());
  return events_[_idx].events;
}
