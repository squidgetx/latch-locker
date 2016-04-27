#include "latch_free_lock_manager.h"
#include <cassert>


bool conflicts(LockRequest o, LockRequest n) {
  if (o.mode_ == EXCLUSIVE) {
    return true;
  }
  if (n.mode_ == EXCLUSIVE) {
    return true;
  }
  return false;
}

TNode<LockRequest>* LatchFreeLockManager::TryWriteLock(TNode<LockRequest> *lr, const Key key) {
  return AcquireLock(lr, key);
}
TNode<LockRequest>* LatchFreeLockManager::WriteLock(TNode<LockRequest> *lr, const Key key) {
  TryWriteLock(lr, key);
  while (lr->data.state_ != ACTIVE){
    do_pause();
    barrier();
}
  return lr;
}
TNode<LockRequest>* LatchFreeLockManager::TryReadLock(TNode<LockRequest> *lr, const Key key) {
  return AcquireLock(lr, key);
}
TNode<LockRequest>* LatchFreeLockManager::ReadLock(TNode<LockRequest> *lr, const Key key) {
  TryReadLock(lr, key);
  while (lr->data.state_ != ACTIVE) {
    do_pause();
    barrier();
}
  return lr;
}

TNode<LockRequest>* LatchFreeLockManager::AcquireLock(TNode<LockRequest> *lr, const Key key) {
  lr->data.state_ = ACTIVE;
  LockRequestLinkedList * list = lock_table.get_list(key);
  list->atomic_lock_insert(lr);
  // iterate over all locks in the chain
  TNode<LockRequest>* req = list->head;
  while (req != NULL && req != lr) {
    if (conflicts(req->data, lr->data)) {
      lr->data.state_ = WAIT;
      barrier();
      if (req->data.state_ == OBSOLETE) {
        lr->data.state_ = ACTIVE;
        barrier();
        req = list->latch_free_next(req);
        continue;
      }
      // deadlock check goes here but we skip this
      // break if we found a conflicting lock
      break;
    }
    req = list->latch_free_next(req);
  }
  return lr;
}

void LatchFreeLockManager::Release(TNode<LockRequest> *lr, const Key key) {
  LockRequestLinkedList * list = lock_table.get_list(key);
  // First find the txn in the lock request list
  TNode<LockRequest> * current;
  // True if txn is the first holder of the lock.
  bool firstLockHolder = true;
  for(current = list->head; current != NULL; current = list->latch_free_next(current)) {
    if (current->data.state_ == OBSOLETE)
      continue;

    if (current->data.txn_ == lr->data.txn_) {
      break;
    }

    firstLockHolder = false;
  }

  if (current == NULL) {
    // abort if the txn doesn't have a lock on this item
    return;
  }

  int mode = current->data.mode_;
  //assert(current->data.state_ == ACTIVE);
  TNode<LockRequest> * next = list->latch_free_next(current);
  while (next != NULL && next->data.state_ == OBSOLETE) {
    next = list->latch_free_next(next);
  }

  current->data.state_ = OBSOLETE;
  barrier();

  if (next == NULL) {
    // nothing left to do if no other txns waiting on this lock
    return;
  }

  if (mode == SHARED) {
    // don't do anything if you aren't the last shared lock holder
    if (next->data.mode_ == SHARED) {
      return;
    }
  }

  // At this point, the released lock was either
  // a) an EXCLUSIVE lock or
  // b) a SHARED lock, AND the next txn is an EXCLUSIVE lock

  if (next->data.mode_ == SHARED) {
    // Grant the lock to all the locks in this segment waiting for
    // a SHARED lock
    for (; next != NULL; next = list->latch_free_next(next)) {
      if (next->data.state_ == OBSOLETE)
        continue;

      if (next->data.mode_ != SHARED) {
        break;
      }

      next->data.state_ = ACTIVE;
      barrier();
    }

  } else if (firstLockHolder) {
    // Just grant to the lock to this request (exclusive lock)
    next->data.state_ = ACTIVE;
    barrier();
  }

}

LockState LatchFreeLockManager::CheckState(const Txn *txn, const Key key) {
  LockRequestLinkedList * list = lock_table.get_list(key);
  TNode<LockRequest> * req = list->head;
  while (req != NULL) {
    if (req->data.txn_ == txn) {
      barrier();
      return req->data.state_;
    }
    req = list->latch_free_next(req);
  }
  return NOT_FOUND;
}
