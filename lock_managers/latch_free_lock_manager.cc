#include "latch_free_lock_manager.h"


bool conflicts(LockRequest o, LockRequest n) {
  if (o.mode_ == EXCLUSIVE) {
    return true;
  }
  if (n.mode_ == EXCLUSIVE) {
    return true;
  }
  return false;
}

bool LatchFreeLockManager::WriteLock(Txn* txn, const Key key) {
  LockRequest n_lock = LockRequest(EXCLUSIVE, txn);
  return AcquireLock(n_lock, key);
}

bool LatchFreeLockManager::ReadLock(Txn* txn, const Key key) {
  LockRequest n_lock = LockRequest(SHARED, txn);
  return AcquireLock(n_lock, key);
}

bool LatchFreeLockManager::AcquireLock(LockRequest n_lock, const Key key) {
  n_lock.state_ = ACTIVE;
  LockRequestLinkedList * list = lock_table.get_list(key);
  TNode<LockRequest>* in = list->atomic_lock_insert(n_lock);
  // iterate over all locks in the chain
  TNode<LockRequest>* req = list->head;
  while (req != NULL && req != in) {
    if (conflicts(req->data, n_lock)) {
      in->data.state_ = WAIT;
      barrier();
      if (req->data.state_ == OBSOLETE) {
        in->data.state_ = ACTIVE;
        barrier();
        continue;
      }
      // deadlock check goes here but we skip this
      // break if we found a conflicting lock
      break;
    }
    req = list->latch_free_next(req);
  }
  //list->printList();
  return in->data.state_ == ACTIVE;
}

void LatchFreeLockManager::Release(Txn* txn, const Key key) {
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
  TNode<LockRequest>* req = list->head;
  TNode<LockRequest>* toRelease;
  bool foundExcl = false;
  bool foundOurTxn = false;
  while (req != NULL) {
    if (req->data.state_ == OBSOLETE) {
      req = list->latch_free_next(req);
      continue;
    }
    if (!foundOurTxn && req->data.txn_ == txn) {
      // If we are a SHARED lock
      //  then any locks after us are already in the correct state
      //  and we can quit
      //If we are an EXCLUSIVE lock AND we found an exclusive lock
      //  then any locks after us are already in the correct state
      // If we are an EXCLUSIVE lock AND we found NO exclusive locks
      //  then we need to grant any shared locks after us UNTIL we find
      //  an exclusive lock (or we are at the end of the list)
      foundOurTxn = true;
      toRelease = req;
      req->data.state_ = OBSOLETE;
      barrier();
      if (foundExcl == true || req->data.mode_ == SHARED) {
        break;
      }
    } else if (req->data.mode_ == EXCLUSIVE) {
      foundExcl = true;
      if (foundOurTxn) {
        break;
      }
    } else if (foundOurTxn) {
      // Grant the lock!
      req->data.state_ = ACTIVE;
      barrier();
    }
    req = list->latch_free_next(req);
  }
  // Clean up
  // list->printList();
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
