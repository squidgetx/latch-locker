#ifndef LOCK_MANAGERS_GLOBAL_LOCK_MANAGER_H
#define LOCK_MANAGERS_GLOBAL_LOCK_MANAGER_H

#include "lock_managers/latched_lock_manager.h"

#include "txn.h"
#include "util/mutex.h"

// Version of the LockManager using a global mutex
class GlobalLockManager : public LatchedLockManager {
 public:
  explicit GlobalLockManager(int nbuckets) : LatchedLockManager(nbuckets) {}
  inline virtual ~GlobalLockManager() {}

  Pthread_mutex KeyMutex(const Key key);
  //virtual LockMode Status(const Key& key, vector<int>* owners);
 protected:
  Pthread_mutex table_mutex;
};

#endif
