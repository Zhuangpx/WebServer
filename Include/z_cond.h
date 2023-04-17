//  <z_cond.h>
//  条件变量

#ifndef _Z_COND_H
#define _Z_COND_H

#include <pthread.h>
#include <exception>

#ifndef COND_OK
#define COND_OK 0
#endif

class zCond
{
public:
  zCond()
  {
    if(pthread_cond_init(&cond_, NULL) != COND_OK )
    {
      std::exception();
    }
  }
  ~zCond()
  {
    pthread_cond_destroy(&cond_);
  }
  bool wait(pthread_mutex_t *mutex)
  {
    int ret = 0;
    pthread_mutex_lock(mutex);
    ret = pthread_cond_wait(&cond_, mutex);
    pthread_mutex_unlock(mutex);
    return ret == COND_OK;
  }
  bool signal()
  {
    return (pthread_cond_signal(&cond_)) == COND_OK;
  }
  bool broadcast()
  {
    return (pthread_cond_broadcast(&cond_)) == COND_OK;
  }
private:
  pthread_cond_t cond_;
};

#endif
