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

TNode<LockRequest>* LatchFreeLockManager::TryWriteLock(Txn* txn, const Key key) {
  LockRequest n_lock = LockRequest(EXCLUSIVE, txn);
  return AcquireLock(n_lock, key);
}
TNode<LockRequest>* LatchFreeLockManager::WriteLock(Txn* txn, const Key key) {
  TNode<LockRequest>* newnode = TryWriteLock(txn, key);
  while (newnode->data.fstate_.state != ACTIVE) do_pause();
  return newnode;
}
TNode<LockRequest>* LatchFreeLockManager::TryReadLock(Txn* txn, const Key key) {
  LockRequest n_lock = LockRequest(SHARED, txn);
  return AcquireLock(n_lock, key);
}
TNode<LockRequest>* LatchFreeLockManager::ReadLock(Txn* txn, const Key key) {
  TNode<LockRequest>* newnode = TryReadLock(txn, key);
  while (newnode->data.fstate_.state != ACTIVE) do_pause();
  return newnode;
}



TNode<LockRequest>* LatchFreeLockManager::AcquireLock(LockRequest n_lock, const Key key) {
  n_lock.fstate_.state = ACTIVE;
  n_lock.fstate_.firstholder = 1;
  LockRequestLinkedList * list = lock_table.get_list(key);
  TNode<LockRequest>* in = list->atomic_lock_insert(n_lock);
  // iterate over all locks in the chain
  TNode<LockRequest>* req = list->head;
  while (req != NULL && req != in) {
    if (conflicts(req->data, n_lock)) {
      in->data.fstate_.state = WAIT;
      in->data.fstate_.firstholder = 0;
      barrier();
      if (req->data.fstate_.state == OBSOLETE) {
        in->data.fstate_.state = ACTIVE;
        in->data.fstate_.firstholder = 1;
        barrier();
        req = list->latch_free_next(req);
        continue;
      }
      // deadlock check goes here but we skip this
      // break if we found a conflicting lock
      break;
    } else if (req->data.fstate_.state != OBSOLETE) {
      in->data.fstate_.firstholder = 0;
    }

    req = list->latch_free_next(req);
  }
  return in;
}

void LatchFreeLockManager::Release(TNode<LockRequest>* req, const Key key) {
//  std::cout << "Releasing txn " << txn->txn_id << std::endl;
  // Find the lock that the txn holds on this key
  LockRequestLinkedList * list = lock_table.get_list(key);
  // Latch free iterate through the list to find the lock for the
  // particular transaction
  // Important to remember the list is ONLY modified before the tail,
  // that is, none of the data we read on this traversal can be modified
  // by other threads with the exception of other threads cleaning up
  // OBSOLETE locks, but we skip this anyway
  //std::cout << "Attempting to release " << txn << " for key " << key << std::endl;
  
  // If we are a SHARED lock
  //  then any locks after us are already in the correct state
  //  and we can quit
  //If we are an EXCLUSIVE lock AND we found an exclusive lock
  //  then any locks after us are already in the correct state
  // If we are an EXCLUSIVE lock AND we found NO exclusive locks
  //  then we need to grant any shared locks after us UNTIL we find
  //  an exclusive lock (or we are at the end of the list)
  assert(req->data.fstate_.state == ACTIVE);
  while (req->data.fstate_.firstholder == 0)
  {
    if (cmp_and_swap(&(req->data.fstate_.full), ACTIVE_NONFIRST, OBSOLETE_NONFIRST)) return;
  }
  //we're the firstholder, so grant the next one and/or pass the bit down...
  TNode<LockRequest>* next = list->latch_free_next(req);
  if (req->data.mode_ == EXCLUSIVE)
  {
    xchgq(&(next->data.fstate_.full), ACTIVE_FIRST);
    if (next->data.mode_ == EXCLUSIVE) return;
    next = list->latch_free_next(next);
    while (next != NULL && next->data.mode_ == SHARED) {
      xchgl(&(next->data.fstate_.state), ACTIVE);
      next = list->latch_free_next(next);
    }
    return;
  }

  //we're in a run of shared locks, so just pass the bit down
  while (next != NULL && next->data.mode_ == SHARED) {
    if (cmp_and_swap(&(next->data.fstate_.full), WAIT_NONFIRST, WAIT_FIRST)) return;
    if (cmp_and_swap(&(next->data.fstate_.full), ACTIVE_NONFIRST, ACTIVE_FIRST)) return;
    next = list->latch_free_next(next);
  }
  if (next == NULL) return;
  //we had a bunch of obsolete shared locks after us, so grant the next exclusive lock;
  xchgq(&(next->data.fstate_.full), ACTIVE_FIRST);
  return;
}


uint32_t LatchFreeLockManager::CheckState(const Txn *txn, const Key key) {
  LockRequestLinkedList * list = lock_table.get_list(key);
  TNode<LockRequest> * req = list->head;
  while (req != NULL) {
    if (req->data.txn_ == txn) {
      barrier();
      return req->data.fstate_.state;
    }
    req = list->latch_free_next(req);
  }
  return NOT_FOUND;
}
