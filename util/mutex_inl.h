#include "mutex.h"

#ifdef UTIL_MUTEX_INL_H
#define UTIL_MUTEX_INL_H

inline void Pthread_mutex::lock() {
    pthread_mutex_lock(&mutex_handle);
}

inline void Pthread_mutex::unlock() {
    pthread_mutex_unlock(&mutex_handle);
}

inline Pthread_mutex_lock Pthread_mutex::lock_handle() {
    return Pthread_mutex_lock(this);
}


Pthread_mutex_lock::Pthread_mutex_lock(Pthread_mutex mutex) : mutex(mutex) {
  mutex.lock();
}

Pthread_mutex_lock::~Pthread_mutex_lock() {
  mutex.unlock();
}

#endif
