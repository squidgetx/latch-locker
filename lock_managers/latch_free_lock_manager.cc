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
 // if (lr->data.state_ == ACTIVE)
   //     fetch_and_increment(&(list->outstanding_locks));
  return lr;
}

void LatchFreeLockManager::Release(TNode<LockRequest> *lr, const Key key) {
  LockRequestLinkedList * list = lock_table.get_list(key);
  // First find the txn in the lock request list
  // latch free iterate until finding 
  
  // Count the active locks until we find our lock
  TNode<LockRequest> * current = list->head;
  int count = 0;
  while(true) {

    if (current == lr) {
      break;
    }

    if (current->data.state_ == OBSOLETE) {
      current = list->latch_free_next(current);
      barrier();
      continue;
    } else if (current->data.state_ == ACTIVE) {
      count++;
    }
    current = list->latch_free_next(current);
    barrier();
  }
 
  lr->data.state_ = OBSOLETE;
  barrier(); 

  if (count == 0) {
    int granted = 0;
    current = list->latch_free_next(current);
    barrier();
    while(current != NULL) {
      // grant locks until we can't anymore
        if (current->data.state_ == OBSOLETE) {
          current = list->latch_free_next(current);
          barrier();
          continue;
        }
        if (current->data.mode_ == EXCLUSIVE) {
          // grant it!
          if (granted == 0) {
            current->data.state_ = ACTIVE;
            barrier();
          }
          break;
        } else {
          current->data.state_ = ACTIVE;
          barrier();
          granted++;
        }
        current = list->latch_free_next(current);
        barrier();
    }
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
