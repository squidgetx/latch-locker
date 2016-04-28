#include <cassert>

#include "lock_managers/latched_lock_manager.h"

#include "util/mutex.h"
#include "txn/txn.h"

TNode<LockRequest>* LatchedLockManager::TryWriteLock(TNode<LockRequest> *lr, const Key key) {
    
    Pthread_mutex_guard guard(KeyMutex(key));

    // add the lock to the queue
    LockRequestLinkedList *list = lock_table.get_list(key);

    lr->data.state_ = WAIT;
    // See if we can get this lock immediately or not
    // If the queue is empty then we can get it, otherwise must wait
    if (list->empty()) {
      lr->data.state_ = ACTIVE;
    }
    list->insertRequest(lr);
    return lr;
    
}
TNode<LockRequest>* LatchedLockManager::WriteLock(TNode<LockRequest> *lr, const Key key) {
  TNode<LockRequest>* newnode = TryWriteLock(lr, key);
  while (newnode->data.state_ != ACTIVE) do_pause();
  return newnode;
}

TNode<LockRequest>* LatchedLockManager::TryReadLock(TNode<LockRequest> *lr, const Key key) {
  
  Pthread_mutex_guard guard(KeyMutex(key));
  
  // add the lock to the queue
  LockRequestLinkedList *list = lock_table.get_list(key);

  lr->data.state_ = ACTIVE;
  // See if we can get this lock immediately or not
  // If the queue is empty then yea
  if (!list->empty()) {
    // If the queue only contains shared locks then we
    // can also get the lock right away
    for (TNode<LockRequest>* node = list->head; node != NULL; node = node->next) {
      const LockRequest ilr = node->data;
      if (ilr.mode_ != SHARED) {
        lr->data.state_ = WAIT;
        break;
      } else {
      }
    }
  }

  return list->insertRequest(lr);;
}

TNode<LockRequest>* LatchedLockManager::ReadLock(TNode<LockRequest> *lr, const Key key) {
  TNode<LockRequest>* newnode = TryReadLock(lr, key);
  while (newnode->data.state_ != ACTIVE) do_pause();
  return newnode;
}

void LatchedLockManager::Release(TNode<LockRequest> *lr, const Key key) {
  Pthread_mutex_guard guard(KeyMutex(key));

  LockRequestLinkedList * list = lock_table.get_list(key);
  if (list == NULL || list->empty()) {
    // don't bother releasing non existent lock
    return;
  }


  // First find the txn in the lock request list
  TNode<LockRequest> * current = lr;
  // True if txn is the first holder of the lock.
  bool firstLockHolder = (lr == list->head);

  if (current == NULL) {
    // abort if the txn doesn't have a lock on this item
    return;
  }

  int mode = current->data.mode_;
  //assert(current->data.state_ == ACTIVE);
  TNode<LockRequest> * next = current->next;
  list->deleteRequest(current);

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
    for (; next != NULL; next = next->next) {
      if (next->data.mode_ != SHARED) {
        break;
      }
    next->data.state_ = ACTIVE;
    }

  } else if (firstLockHolder) {
    // Just grant to the lock to this request (exclusive lock)
    next->data.state_ = ACTIVE;
  }

}

LockState LatchedLockManager::CheckState(const Txn *txn, const Key key) {
  Pthread_mutex_guard guard(KeyMutex(key));

  TNode<LockRequest> *node = lock_table.get_list(key)->head;
  for (; node != NULL; node = node->next) {
    if (node->data.txn_ == txn) {
      return node->data.state_;
    }
  }

  return NOT_FOUND;
}
