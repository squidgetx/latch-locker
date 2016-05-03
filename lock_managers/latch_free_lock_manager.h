#ifndef _LATCHFREE_LOCK_MANAGER_H_
#define _LATCHFREE_LOCK_MANAGER_H_

#include "lock_managers/lock_manager.h"
#include "util/util.h"

// Version of the lock manager using the latch free algorithm
class LatchFreeLockManager : public LockManager {
  public:
    explicit LatchFreeLockManager(int nbuckets) : LockManager(nbuckets) {}
    inline virtual ~LatchFreeLockManager() {}

    TNode<LockRequest>* ReadLock(TNode<LockRequest> *lr, const Key key);
    TNode<LockRequest>* WriteLock(TNode<LockRequest> *lr, const Key key);
    TNode<LockRequest>* TryReadLock(TNode<LockRequest> *lr, const Key key);
    TNode<LockRequest>* TryWriteLock(TNode<LockRequest> *lr, const Key key);

    void Release(TNode<LockRequest> *lr, const Key key);
    virtual LockState CheckState(const Txn *txn, const Key key);
  private:
    TNode<LockRequest>* AcquireLock(TNode<LockRequest> *lr, const Key key);


};

#endif


