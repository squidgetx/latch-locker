#ifndef _GLOBAL_LOCK_MANAGER_H_
#define _GLOBAL_LOCK_MANAGER_H_

#include "lock_manager.h"

// Version of the LockManager using a global mutex
class GlobalLockManager : public LockManager {
 public:
  explicit GlobalLockManager();
  inline virtual ~GlobalLockManager() {}

  virtual bool ReadLock(Txn* txn, const Key key);
  virtual bool WriteLock(Txn* txn, const Key key);
  virtual void Release(Txn* txn, const Key key);
  //virtual LockMode Status(const Key& key, vector<int>* owners);
 protected:
  Pthread_mutex table_mutex;

};

#endif
