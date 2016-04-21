#include "lock_managers/key_lock_manager.h"

Pthread_mutex KeyLockManager::KeyMutex(const Key key) {
  return lock_table.mutex(key);
}
