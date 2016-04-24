#ifndef _LATCHFREE_LOCK_MANAGER_H_
#define _LATCHFREE_LOCK_MANAGER_H_

#include "lock_managers/lock_manager.h"
#include "util/util.h"

// Version of the lock manager using the latch free algorithm
class LatchFreeLockManager : public LockManager {
  public:
    explicit LatchFreeLockManager(int nbuckets) : LockManager(nbuckets) {}
    inline virtual ~LatchFreeLockManager() {}

    virtual bool ReadLock(Txn* txn, const Key key);
    virtual bool WriteLock(Txn* txn, const Key key);
    virtual void Release(Txn* txn, const Key key);
  private:
    bool AcquireLock(LockRequest n_lock, const Key key);

};

#endif


