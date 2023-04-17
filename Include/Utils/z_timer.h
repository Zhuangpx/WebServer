//  <z_timer.h>
//  timer 计时器 给HTTP连接附加定时器，过期清理
//  小根堆管理定时器

#ifndef _Z_TIMER_H
#define _Z_TIMER_H

#include <queue>
#include <time.h>
#include <map>                 //  map
#include <vector>              //  vector
#include <algorithm>           //  function
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>              //  chrono
#include "../Log/z_log.h"

typedef std::function<void()> TimeoutCallBack;    //  call back function
typedef std::chrono::high_resolution_clock Clock; //  time clock
typedef std::chrono::milliseconds MS;             //  ms
typedef Clock::time_point TimeStamp;              //  time point

struct zTimerNode
{
  int id_;             //  http connect id (socket)
  TimeStamp expires_;  //  expiration time
  TimeoutCallBack cb_; //  call back when time out (function)
  //  overload operator <
  inline bool operator < (const zTimerNode& _t) { return expires_ < _t.expires_; }
};

class zTimer
{
public:
  zTimer() { timer_heap_.reserve(64); }
  ~zTimer() { clear(); }
  void pop();                                                   //  pop the recently timer (next_tick)
  void clear();                                                 //  clear timer list
  void tick();                                                  //  tick timer
  int next_tick();                                              //  get the next timeout (next_tick)
  void adjust(int _idx, int _new_timeout);                      //  relay the expiration time by _new_timeout
  void add(int _id, int _timeout, const TimeoutCallBack &_cb);  //  add timer
  bool do_work(int _id);                                        //  do work

private:
  //  ! heap's idx use form '0'
  void heap_swap(size_t _idxf, size_t _idxs);  //  swap TimerNode
  void heap_siftup(size_t _idx);               //  siftup TimerNode
  bool heap_siftdown(size_t _idx, size_t _n);  //  siftdown TimerNode
  void heap_del(size_t _idx);                  //  delete TimerNode
  std::vector<zTimerNode> timer_heap_;         //  timer heap
  std::map<int, size_t> ref_;                  //  TimerNode.id_ -> heap's idx
};


#endif
