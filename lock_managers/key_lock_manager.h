#ifndef _KEYLOCK_MANAGER_H_
#define _KEYLOCK_MANAGER_H_

#include "lock_managers/latched_lock_manager.h"

#include "util/common.h"
#include "util/mutex.h"

// Version of the LockManager using a global mutex
class KeyLockManager : public LatchedLockManager {
 public:
  explicit KeyLockManager(int nbuckets) : LatchedLockManager(nbuckets) {}
  inline virtual ~KeyLockManager() {}

  Pthread_mutex& KeyMutex(const Key key);
  //virtual LockMode Status(const Key& key, vector<int>* owners);
};

#endif

