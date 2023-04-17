#include "z_timer.h"

/**
 * public:
*/

//  计时器 删除堆中最近计算器
void zTimer::pop()
{
  assert(!timer_heap_.empty());
  heap_del(0);
}

//  计时器 清空计时器列表
void zTimer::clear()
{
  timer_heap_.clear();
  ref_.clear();
}

//  计时器 处理超时计时器
void zTimer::tick()
{
  if(timer_heap_.empty())
  { return ; }
  while(!timer_heap_.empty())
  {
    zTimerNode _node = timer_heap_.front();
    if(std::chrono::duration_cast<MS>(_node.expires_ - Clock::now()).count() > 0)
    { break; }
    _node.cb_();
    pop();
  }
}

//  计算器 取最近一次计时器的时间戳间隔
int zTimer::next_tick()
{
  tick();
  int _next = -1;
  if(!timer_heap_.empty())
  {
    _next = std::chrono::duration_cast<MS>(timer_heap_.front().expires_ - Clock::now()).count();
    if(_next < 0) _next = 0;
  }
  return _next;
}

//  计时器 调整指定计时器的时间戳
void zTimer::adjust(int _id, int _new_timeout)
{
  assert(!timer_heap_.empty() && ref_.count(_id) > 0);
  timer_heap_[ref_[_id]].expires_ = Clock::now() + MS(_new_timeout);
  heap_siftdown(ref_[_id], timer_heap_.size());
}

//  计时器 往计时器列表添加计时器
void zTimer::add(int _id, int _timeout, const TimeoutCallBack &_cb)
{
  assert(_id >= 0);
  size_t _idxi;
  if(ref_.count(_id) == 0)
  {
    _idxi = timer_heap_.size();
    ref_[_id] = _idxi;
    timer_heap_.push_back({_id, Clock::now() + MS(_timeout), _cb});
    heap_siftup(_idxi);
  }
  else
  {
    _idxi = ref_[_id];
    timer_heap_[_idxi].expires_ = Clock::now() + MS(_timeout);
    timer_heap_[_idxi].cb_ = _cb;
    if(!heap_siftdown(_idxi, timer_heap_.size()))
    { heap_siftup(_idxi); }
  }
}

//  计时器 主动触发 回调后删除
bool zTimer::do_work(int _id)
{
  if(timer_heap_.empty() || ref_.count(_id) == 0)
  { return false; }
  size_t _idx = ref_[_id];
  zTimerNode _node = timer_heap_[_idx];
  _node.cb_();
  heap_del(_idx);
  return true;
}


/**
 * private:
*/

//  堆 交换结点
void zTimer::heap_swap(size_t _idxf, size_t _idxs)
{
  assert(_idxf >= 0 && _idxf < timer_heap_.size());
  assert(_idxs >= 0 && _idxs < timer_heap_.size());
  std::swap(timer_heap_[_idxf], timer_heap_[_idxs]);
  ref_[timer_heap_[_idxf].id_] = _idxf;
  ref_[timer_heap_[_idxs].id_] = _idxs;
}

//  堆 上调整
void zTimer::heap_siftup(size_t _idx)
{
  assert(_idx >= 0 && _idx < timer_heap_.size());
  size_t _idxs = (_idx - 1) / 2;
  while(_idxs >= 0)
  {
    if(timer_heap_[_idxs] < timer_heap_[_idx])
    { break; }
    heap_swap(_idx, _idxs);
    _idx = _idxs;
    _idxs = (_idx - 1) / 2;
  }
}

//  堆 下调整 [_idx, _n)
bool zTimer::heap_siftdown(size_t _idx, size_t _n)
{
  assert(_idx >= 0 && _idx < timer_heap_.size());
  assert(_n >= 0 && _n <= timer_heap_.size());
  size_t _idxi = _idx, _idxj = _idx*2 + 1;
  while(_idxj < _n)
  {
    if(_idxj + 1 < _n && timer_heap_[_idxj + 1] < timer_heap_[_idxj])  ++_idxj;
    if(timer_heap_[_idxi] < timer_heap_[_idxj])
    { break; }
    heap_swap(_idxi, _idxj);
    _idxi = _idxj;
    _idxj = _idxi * 2 + 1;
  }
  return _idxi > _idx;
}

//  堆 删除结点
void zTimer::heap_del(size_t _idx)
{
  //  删除指定位置的结点  将要删除的结点换到队尾，然后调整堆
  assert(!timer_heap_.empty() && _idx >= 0 && _idx < timer_heap_.size());
  size_t _idx_back = timer_heap_.size() - 1;
  if(_idx < _idx_back) {
    heap_swap(_idx, _idx_back);
    if(!heap_siftdown(_idx, _idx_back))
    { heap_siftup(_idx); }
  }
  ref_.erase(timer_heap_.back().id_);
  timer_heap_.pop_back();
}

