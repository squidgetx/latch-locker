#ifndef LOCK_MANAGERS_LATCHED_LOCK_MANAGER
#define LOCK_MANAGERS_LATCHED_LOCK_MANAGER

#include "lock_managers/lock_manager.h"
#include "util/mutex.h"
#include "util/common.h"
#include "txn.h"

// Version of the LockManager using a global mutex
class LatchedLockManager : public LockManager {
 public:
  explicit LatchedLockManager(int nbuckets) : LockManager(nbuckets) {}
  inline virtual ~LatchedLockManager() {}

  virtual bool ReadLock(Txn* txn, const Key key);
  virtual bool WriteLock(Txn* txn, const Key key);
  virtual void Release(Txn* txn, const Key key);
  //virtual LockMode Status(const Key& key, vector<int>* owners);
  virtual LockState CheckState(const Txn *txn, const Key key);

  /**
   * Get a guard for the key at @key.
   */
  virtual Pthread_mutex& KeyMutex(const Key key) = 0;
};

#endif // LOCK_MANAGERS_LATCHED_LOCK_MANAGER
