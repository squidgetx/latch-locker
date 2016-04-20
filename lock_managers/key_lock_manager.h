#ifndef _KEYLOCK_MANAGER_H_
#define _KEYLOCK_MANAGER_H_

#include "lock_manager.h"

// Version of the LockManager using a global mutex
class KeyLockManager : public LockManager {
 public:
  explicit KeyLockManager();
  inline virtual ~KeyLockManager() {}

  virtual bool ReadLock(Txn* txn, const Key key);
  virtual bool WriteLock(Txn* txn, const Key key);
  virtual void Release(Txn* txn, const Key key);
  //virtual LockMode Status(const Key& key, vector<int>* owners);
 protected:
  Pthread_mutex table_mutex;

};

#endif

