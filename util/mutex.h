/**
 * @author David Hatch
 */
#ifndef UTIL_MUTEX_H
#define UTIL_MUTEX_H


#include <pthread.h>

class Pthread_mutex_lock;

/**
 * Provides mutual exclusion, using pthread.
 */
class Pthread_mutex {
public:
  Pthread_mutex();
  inline void lock() {
    pthread_mutex_lock(&mutex_handle);
  }
  inline void unlock() {
    pthread_mutex_unlock(&mutex_handle);
  }
private:
  pthread_mutex_t mutex_handle;
};

/**
 * RAII lock, based on Pthread_mutex
 */
class Pthread_mutex_guard {
public:
  Pthread_mutex_guard(Pthread_mutex mutex) : mutex(mutex) {
    mutex.lock();
  }
  ~Pthread_mutex_guard() {
    mutex.unlock();
  }

  Pthread_mutex_guard operator=(const Pthread_mutex_guard&) = delete;
private:
  Pthread_mutex mutex;
};

#endif
