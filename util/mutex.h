/**
 * @author David Hatch
 */
#ifndef UTIL_MUTEX_H
#define UTIL_MUTEX_H


#include <pthread.h>

#include "mutex_inl.h"

class Pthread_mutex_lock;

/**
 * Provides mutual exclusion, using pthread.
 */
class Pthread_mutex {
public:
  Pthread_mutex();
  inline void lock();
  inline void unlock();

  /**
   * Lock the mutex, and acquire a RAII handle that unlocks when it goes out
   * of scope.
   */
  inline class Pthread_mutex_lock lock_handle();
private:
  pthread_mutex_t mutex_handle;
};

/**
 * RAII lock, based on Pthread_mutex
 */
class Pthread_mutex_lock {
public:
  Pthread_mutex_lock(Pthread_mutex mutex);
  ~Pthread_mutex_lock();
private:
  Pthread_mutex mutex;
};


#endif
