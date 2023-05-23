#ifndef __threadwrapper_h__
#define __threadwrapper_h__

#include <pthread.h>

class ThreadWrapper {
  public:
    ThreadWrapper();
    virtual ~ThreadWrapper();

    int start();
    int join();
    int detach();
    pthread_t self();
    
    virtual void* run() = 0;
    
  private:
    pthread_t  m_tid;
    int        m_running;
    int        m_detached;
};

#endif