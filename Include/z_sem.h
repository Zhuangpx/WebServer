//  <z_sem.h>
//  信号量 只提供给多线程工作

#ifndef _Z_SEM_H
#define _Z_SEM_H

#include <semaphore.h>
#include <exception>

#ifndef SEM_OK
#define SEM_OK 0
#endif

class zSem
{
/**
 * zSem 信号量
 * 为多线程工作
 */
public:
  zSem()
  {
    if (sem_init(&sem_, 0, 0) != SEM_OK)
    {
      throw std::exception();
    }
  }
  zSem(unsigned int value)
  {
    if (sem_init(&sem_, 0, value) != SEM_OK)
    {
      throw std::exception();
    }
  }
  ~zSem()
  {
    sem_destroy(&sem_);
  }
  bool wait()
  {
    return sem_wait(&sem_);
  }
  bool post()
  {
    return sem_post(&sem_);
  }

private:
  sem_t sem_;
};

#endif
