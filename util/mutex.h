/**
 * @author David Hatch
 */
#ifndef UTIL_MUTEX_H
#define UTIL_MUTEX_H


#include <pthread.h>

/**
 * Provides mutual exclusion, using pthread.
 */
class Pthread_mutex {
public:
  Pthread_mutex() {
    pthread_mutex_init(&mutex_handle, NULL);
  }

  void lock() {
    pthread_mutex_lock(&mutex_handle);
  }

  void unlock() {
    pthread_mutex_unlock(&mutex_handle);
  }
private:
  // Wow this is janky.
  pthread_mutex_t mutex_handle;
};

/**
 * RAII guard, based on Pthread_mutex
 */
class Pthread_mutex_guard {
public:
  Pthread_mutex_guard(Pthread_mutex mutex) : mutex(mutex) {
    mutex.lock();
  }

  ~Pthread_mutex_guard() {
    mutex.unlock();
  }

  Pthread_mutex_guard& operator=(const Pthread_mutex_guard& rhs) = delete;
private:
  Pthread_mutex mutex;
};

#endif // UTIL_MUTEX_H
