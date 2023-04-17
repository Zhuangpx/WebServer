//  <z_block_deque.h>
//  队列模拟生产者消费者模型

#ifndef _Z_BLOCK_DEQUE_H
#define _Z_BLOCK_DEQUE_H


#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>
#include <vector>
#include <assert.h>

template<typename _Tp>
class zBlockDeque
{
public:
  explicit zBlockDeque(size_t _max_capacity = 1000);

  ~zBlockDeque();

  void clear();

  bool empty();

  bool full();

  void close();

  size_t size();

  size_t capacity();

  _Tp front();

  _Tp back();

  void push_back(const _Tp& _item);

  void push_front(const _Tp& _item);

  bool pop(_Tp& _item);

  bool pop(_Tp& _item, int _timeout);

  void flush();

private:
  std::deque<_Tp> deq_;

  size_t capacity_;

  std::mutex mtx_;

  bool is_close_;

  std::condition_variable cond_consumer_;

  std::condition_variable cond_producer_;
};


template <typename _Tp>
zBlockDeque<_Tp>::zBlockDeque(size_t _max_capacity) : capacity_(_max_capacity)
{
  assert(_max_capacity > 0);
  is_close_ = false;
}

template <typename _Tp>
zBlockDeque<_Tp>::~zBlockDeque()
{
  close();
};

template <typename _Tp>
void zBlockDeque<_Tp>::close()
{
  std::lock_guard<std::mutex> locker(mtx_);
  deq_.clear();
  is_close_ = true;
  cond_producer_.notify_all();
  cond_consumer_.notify_all();
};

template <typename _Tp>
void zBlockDeque<_Tp>::flush()
{
  cond_consumer_.notify_one();
};

template <typename _Tp>
void zBlockDeque<_Tp>::clear()
{
  std::lock_guard<std::mutex> locker(mtx_);
  deq_.clear();
}

template <typename _Tp>
_Tp zBlockDeque<_Tp>::front()
{
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.front();
}

template <typename _Tp>
_Tp zBlockDeque<_Tp>::back()
{
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.back();
}

template <typename _Tp>
size_t zBlockDeque<_Tp>::size()
{
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size();
}

template <typename _Tp>
size_t zBlockDeque<_Tp>::capacity()
{
  std::lock_guard<std::mutex> locker(mtx_);
  return capacity_;
}

template <typename _Tp>
void zBlockDeque<_Tp>::push_back(const _Tp& item)
{
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.size() >= capacity_)
  {
    cond_producer_.wait(locker);
  }
  deq_.push_back(item);
  cond_consumer_.notify_one();
}

template <typename _Tp>
void zBlockDeque<_Tp>::push_front(const _Tp& item)
{
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.size() >= capacity_)
  {
    cond_producer_.wait(locker);
  }
  deq_.push_front(item);
  cond_consumer_.notify_one();
}

template <typename _Tp>
bool zBlockDeque<_Tp>::empty()
{
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.empty();
}

template <typename _Tp>
bool zBlockDeque<_Tp>::full()
{
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size() >= capacity_;
}

template <typename _Tp>
bool zBlockDeque<_Tp>::pop(_Tp& item)
{
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty())
  {
    cond_consumer_.wait(locker);
    if (is_close_)
    {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  cond_producer_.notify_one();
  return true;
}

template <typename _Tp>
bool zBlockDeque<_Tp>::pop(_Tp& item, int timeout)
{
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty())
  {
    if (cond_consumer_.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout)
    {
      return false;
    }
    if (is_close_)
    {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  cond_producer_.notify_one();
  return true;
}

#endif  //  _Z_BLOCK_DEQUE_H
