#ifndef _LATCHFREE_LOCK_MANAGER_H_
#define _LATCHFREE_LOCK_MANAGER_H_

// Version of the lock manager using the latch free algorithm
class LatchFreeLockManager : public LockManager {
  public:
    explicit LatchFreeLockManager() : LockManager(int nbuckets);
    inline virtual ~LatchFreeLockManager() {}

    virtual bool ReadLock(Txn* txn, const Key key);
    virtual bool WriteLock(Txn* txn, const Key key);
    virtual void Release(Txn* txn, const Key key);
};

#endif


