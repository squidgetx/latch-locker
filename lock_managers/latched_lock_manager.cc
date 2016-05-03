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
      ++(list->outstanding_locks);
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
  if (lr->data.state_ == ACTIVE) ++(list->outstanding_locks);

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
  list->deleteRequest(lr);
  if (--(list->outstanding_locks) > 0) return; //other shared locks still outstanding
  TNode<LockRequest>* next = lr->next;

  if (next == NULL) {
    // nothing left to do if no other txns waiting on this lock
    return;
  }
  if (next->data.mode_ == EXCLUSIVE) {
    ++(list->outstanding_locks);
    next->data.state_ = ACTIVE;
    return;
  }
  else {
    for (; next != NULL && next->data.mode_ == SHARED; next = next->next) {
      ++(list->outstanding_locks);
      next->data.state_ = ACTIVE;
    }
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
