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
        fetch_and_increment(&(list->outstanding_locks));
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
  lr->data.state_ = OBSOLETE;
  barrier();
  uint64_t num_locks = fetch_and_decrement(&(list->outstanding_locks));
  if (num_locks > 0) return; //other shared locks still outstanding
  TNode<LockRequest>* next = list->latch_free_next(lr);

  if (next == NULL) {
    // nothing left to do if no other txns waiting on this lock
    return;
  }
  if (next->data.mode_ == EXCLUSIVE) {
    fetch_and_increment(&(list->outstanding_locks));
    next->data.state_ = ACTIVE;
    barrier();
    return;
  }
  else {
    for (; next != NULL && next->data.mode_ == SHARED; next = list->latch_free_next(next)) {
      fetch_and_increment(&(list->outstanding_locks));
      next->data.state_ = ACTIVE;
      barrier();
    }
  }
  return;
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
