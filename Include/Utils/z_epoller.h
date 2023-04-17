//  <z_epoller.h>
//  epoll IO多路复用 采用事件驱动的方式监听IO

#ifndef _Z_EPOLLER_H
#define _Z_EPOLLER_H

#include <vector>      //  vector
#include <sys/epoll.h> //  epoll_ctl() | uint32_t
#include <fcntl.h>     //  fcntl()
#include <unistd.h>    //  close()
#include <assert.h>    //  assert()
#include <errno.h>

class zEpoller
{
public:
  explicit zEpoller(int max_event = 1024);
  ~zEpoller();
  bool add_fd(int _fd, uint32_t _events);    //  add _fd
  bool modify_fd(int _fd, uint32_t _events); //  modify _fd
  bool delete_fd(int _fd);                   //  delete _fd
  int wait(int _timeout = -1);               //  listen and wait
  int get_eventfd(size_t _idx) const;        //  get event_fd
  uint32_t get_events(size_t _idx) const;    //  get events
private:
  int epoll_fd_;                             //  flag of epoll_fd
  std::vector<struct epoll_event> events_;   //  ready events
};

#endif

/**
 * Stills & Src:
*/
