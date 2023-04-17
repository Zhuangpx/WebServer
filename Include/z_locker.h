//  <z_locker.h>
//  锁

#ifndef _Z_LOCKER_H
#define _Z_LOCKER_H

#include <pthread.h>
#include <exception>

#ifndef LOCKER_OK
#define LOCKER_OK 0
#endif

class zLocker
{
/**
 * zLocker 锁
 */
public:
  zLocker()
  {
    if(pthread_mutex_init(&mutex_, NULL) != LOCKER_OK )
    {
      throw std::exception();
    }
  }
  ~zLocker()
  {
    pthread_mutex_destroy(&mutex_);
  }
  bool lock()
  {
    return pthread_mutex_lock(&mutex_);
  }
  bool unlock()
  {
    return pthread_mutex_unlock(&mutex_);
  }
  pthread_mutex_t *get_mutex_p()
  {
    return &mutex_;
  }

private:
  pthread_mutex_t mutex_;
};

#endif
