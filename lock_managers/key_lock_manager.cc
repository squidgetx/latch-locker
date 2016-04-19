#include "lock_manager.h"

KeyLockManager::KeyLockManager() {

}

bool KeyLockManager::WriteLock(Txn* txn, const Key key) {
  lock_table_.lock(key);

  deque<LockRequest> * list = lock_table_[key];
  // For now, assume the entry will always exist

  // add the lock to the queue
  LockRequest * newreq = new LockRequest(EXCLUSIVE, txn);

  bool wasEmpty = list->empty();
  if (wasEmpty) {
    newreq->state_ = ACTIVE;
  } else {
    newreq->state_ = WAIT;
  }

  list->push_back(*newreq);

  lock_table.unlock(key);
  return wasEmpty;
}

bool KeyLockManager::ReadLock(Txn* txn, const Key key) {
  lock_table.lock(key);

  deque<LockRequest> * list = lock_table_[key];

  // add the lock to the queue
  LockRequest * newreq = new LockRequest(SHARED, txn);
  list->push_back(*newreq);

  newreq->state_ = WAIT;
  // See if we can get this lock immediately or not
  // If the queue is empty then yea
  if (!list->empty()) {
    // If the queue only contains shared locks then we
    // can also get the lock right away
    for(deque<LockRequest>::iterator i = list->begin(); i != list->end(); i++) {
      if (i->mode_ != SHARED) {
        newreq->state_ = ACTIVE;
        break;
      }
    }
  } else {
    newreq->state_ = ACTIVE;
  }
  table_mutex.unlock();

  return (newreq->state_ == ACTIVE);
}


void KeyLockManager::Release(Txn* txn, const Key key) {
  table_mutex.lock();

  deque<LockRequest> * list = lock_table_[key];
  if (list == NULL || list->empty()) {
    // don't bother releasing non existent lock
    table_mutex.unlock();
    return;
  }

  // First find the txn in the lock request list
  deque<LockRequest>::iterator current;
  for(current = list->begin(); current != list->end(); current++) {
    if (current->txn_ == txn) {
      break;
    }
  }

  if (current == list->end()) {
    // abort if the txn doesn't have a lock on this item
    table_mutex.unlock();
    return;
  }

  int mode = current->mode_;
  deque<LockRequest>::iterator next = list->erase(current);

  if (next == list->end()) {
    // nothing left to do if no other txns waiting on this lock
    table_mutex.unlock();
    return;
  }

  if (mode == SHARED) {
    // don't do anything if you aren't the last shared lock holder
    if (next->mode_ == SHARED) {
      table_mutex.unlock();
      return;
    }
  }

  // At this point, the released lock was either
  // a) an EXCLUSIVE lock or
  // b) a SHARED lock, AND the next txn is an EXCLUSIVE lock

  if (next->mode_ == SHARED) {
    // Grant the lock to all the locks in this segment waiting for
    // a SHARED lock
    for(deque<LockRequest>::iterator i = next; i != list->end(); i++) {
      if (i->mode_ != SHARED) {
        break;
      }
      i->state_ = ACTIVE;
    }

  } else {
    // Just grant to the lock to this request (exclusive lock)
    next->state_ = ACTIVE;
  }

  table_mutex.unlock();
}
