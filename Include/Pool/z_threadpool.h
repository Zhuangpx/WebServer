//  <z_threadpool>
//  threadpool 线程池 预处理的静态资源池

#ifndef _Z_THREADPOOL_H
#define _Z_THREADPOOL_H


#include <mutex>              //  mutex
#include <condition_variable> //  condition_variable
#include <queue>              //  queue
#include <functional>         //  function
#include <assert.h>           //  assert
#include <thread>             //  thread

class zThreadPool
{
public:

  zThreadPool() = default;

  zThreadPool(zThreadPool&&) = default;

  explicit zThreadPool(size_t _thread_cnt = 8) : pool_ptr_(std::make_shared<zPool>())
  {
    assert(_thread_cnt > 0);
    //  range and thread detach
    for(size_t _i=0; _i<_thread_cnt; _i++) {
      //  std::function<void()> 做参数 构造线程并分离
      std::thread(
        [_pool_ptr = pool_ptr_]()
        {
          std::unique_lock<std::mutex> _locker(_pool_ptr->mtx_);
          while(true)
          {
            if(!_pool_ptr->tasks_.empty())
            {
              auto _task = std::move(_pool_ptr->tasks_.front());  //  注意是std::move 而不是直接front()
              _pool_ptr->tasks_.pop();
              _locker.unlock();
              _task();
              _locker.lock();
            }
            else if(_pool_ptr->closed_fl_)
            { break; }
            else
            { _pool_ptr->cond_.wait(_locker); }                   //  未关闭且为空 需等待条件变量
          }
        }
      ).detach();
    }
  }

  ~zThreadPool()
  {
    //  关闭 并且通知条件变量
    if(static_cast<bool>(pool_ptr_))
    {
      std::unique_lock<std::mutex>(pool_ptr_->mtx_);
      pool_ptr_->closed_fl_ = true;
      pool_ptr_->cond_.notify_all();
    }
  }

  template<typename __Func>
  void add_task(__Func&& _task)
  {
    std::lock_guard<std::mutex> _locker(pool_ptr_->mtx_);
    pool_ptr_->tasks_.emplace(std::forward<__Func>(_task));
    pool_ptr_->cond_.notify_one();
  }

private:

  struct zPool
  {
    bool closed_fl_;                          //  closed flag
    std::mutex mtx_;                          //  mutex
    std::condition_variable cond_;            //  condition
    std::queue<std::function<void()>> tasks_; //  task list
  };

  std::shared_ptr<zPool> pool_ptr_;

};



#endif  //  _Z_THREADPOOL_H


/**
 * Stills & Src:
 * mutex & condition_variable
 * shared_ptr
 * 通用引用
*/
