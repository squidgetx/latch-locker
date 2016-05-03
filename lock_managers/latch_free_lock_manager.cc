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
  lr->data.state_ = WAIT;
  LockRequestLinkedList * list = lock_table.get_list(key);
  list->atomic_lock_insert(lr);
  // iterate over all locks in the chain
  TNode<LockRequest>* req = list->head;
  bool need_to_wait = false;
  while (req != NULL && req != lr) {
    if (conflicts(req->data, lr->data)) {
      need_to_wait = true;
      barrier();
      if (req->data.state_ == OBSOLETE) {
        if (lr->data.state_ == WAIT) {
          need_to_wait = false;
          req = list->latch_free_next(req);
          continue;
        }
        else {
          break;
        }
      }
      // deadlock check goes here but we skip this
      // break if we found a conflicting lock
      break;
    }
    req = list->latch_free_next(req);
  }
  if (!need_to_wait) {
    lr->data.state_ = ACTIVE;
    barrier();
    fetch_and_increment(&(list->outstanding_locks));
  }
  return lr;
}

void LatchFreeLockManager::Release(TNode<LockRequest> *lr, const Key key) {
  LockRequestLinkedList * list = lock_table.get_list(key);
  uint64_t num_locks = fetch_and_decrement(&(list->outstanding_locks));
  if (num_locks == 0) {
    TNode<LockRequest>* next = list->latch_free_next(lr);
    if (next != NULL) {
     
      if (next->data.mode_ == EXCLUSIVE) {
        fetch_and_increment(&(list->outstanding_locks));
        next->data.state_ = ACTIVE;
        barrier();
      }
      else {
        for (; next != NULL && next->data.mode_ == SHARED; next = list->latch_free_next(next)) {
          fetch_and_increment(&(list->outstanding_locks));
          next->data.state_ = ACTIVE;
          barrier();
        }
      }
    }
  }
  lr->data.state_ = OBSOLETE; 
  barrier();

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
