#include "latch_free_lock_manager.h"

#define MODULUS 5

LatchFreeLockManager::LatchFreeLockManager() {
  // Allocate memory for the hash table
  

}


bool conflicts(LockRequest * o, LockRequest * n) {
  if (o_>state_ == OBSOLETE) {
    return false;
  }
  if (o->mode_ == EXCLUSIVE) {
    return true;
  }
  if (n->mode_ == EXCLUSIVE) {
    return true;
  }
  return false;
}

bool LatchFreeLockManager::WriteLock(Txn* txn, const Key key) {
  // TODO don't dynamically create this
  LockRequest * n_lock = new LockRequest(EXCLUSIVE, txn);
  return AcquireLock(n_lock);
}

bool LatchFreeLockManager::ReadLock(Txn* txn, const Key key) {
  LockRequest * n_lock = new LockRequest(SHARED, txn);
  return AcquireLock(n_lock);
}

bool LatchFreeLockManager::AcquireLock(LockRequest n_lock) {
  n_lock->state_ = ACTIVE;
  int bucket = hash(key);
  LockRequestLinkedList * list = locks[bucket];
  list->atomic_lock_insert(n_lock);
  // iterate over all locks in the chain
  LockRequest * req = list->head();
  while (req != NULL) {
    if (conflicts(req, n_lock)) {
      n_lock->state_ = WAIT;
      atomic_synchronize();
      if (req->state_ == OBSOLETE) {
        n_lock->state_ = ACTIVE;
        atomic_synchronize();
        continue;
      }
      // deadlock check goes here but we skip this
      // break if we found a conflicting lock
      break;
    }
    req = list->latch_free_iterate(req);
  }
  return n_lock->state_ == ACTIVE;
}

void LatchFreeLockManager::Release(Txn* txn, const Key key) {
  // Find the lock that the txn holds on this key
  LockRequestLinkedList * list = locks[hash(key)];
  // Latch free iterate through the list to find the lock for the
  // particular transaction
  LockRequest req = list->head();
  // TODO why does release code suck so much
  // First find the lock
  while (req != null) {
    if (req->txn_ == txn) {
      request_release(req);
      break;
    }
    req = latch_free_iterate(req);
  }
}

void LatchFreeLockManager::request_release(lock) {
  LockState oldState = lock->state_;
  LockMode oldMode = lock->mode_;
    // TODO is there a concurrency issue here with reading these fields??
  lock->state_ = OBSOLETE;
  atomic_synchronize();
  LockRequest req = latch_free_iterate(lock);
  bool activateNext = false;
  if (oldState == ACTIVE) {
    activateNext = true;
  } else {
    // If all the previous locks were SHARED and this one was exclusive,
    // then if a SHARED follows it should be activated...
    // fuck this

  }
  while (req != null) {
    if (req->state == WAIT) {

    }


    req = latch_free_iterate(req);
  }




}
