#include "global_lock_manager.h"
#include "util/mutex.h"
#include "util/common.h"

Pthread_mutex& GlobalLockManager::KeyMutex(const Key key) {
  return table_mutex;
}
