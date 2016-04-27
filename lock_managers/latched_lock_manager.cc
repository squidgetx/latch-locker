#include <cassert>

#include "lock_managers/latched_lock_manager.h"

#include "util/mutex.h"
#include "txn/txn.h"

TNode<LockRequest>* LatchedLockManager::TryWriteLock(Txn* txn, const Key key) {
    LockRequest newreq = LockRequest(EXCLUSIVE, txn);
    TNode<LockRequest>* newnode;
    
    Pthread_mutex_guard guard(KeyMutex(key));

    // add the lock to the queue
    LockRequestLinkedList *list = lock_table.get_list(key);

    newreq.state_ = WAIT;
    // See if we can get this lock immediately or not
    // If the queue is empty then we can get it, otherwise must wait
    if (list->empty()) {
      newreq.state_ = ACTIVE;
    }

    newnode = list->insertRequest(newreq);
    return newnode;
    
}
TNode<LockRequest>* LatchedLockManager::WriteLock(Txn* txn, const Key key) {
  TNode<LockRequest>* newnode = TryWriteLock(txn, key);
  while (newnode->data.state_ != ACTIVE) do_pause();
  return newnode;
}

TNode<LockRequest>* LatchedLockManager::TryReadLock(Txn* txn, const Key key) {
  LockRequest newreq = LockRequest(SHARED, txn);
  TNode<LockRequest>* newnode;
  
  Pthread_mutex_guard guard(KeyMutex(key));
  
  // add the lock to the queue
  LockRequestLinkedList *list = lock_table.get_list(key);

  newreq.state_ = ACTIVE;
  // See if we can get this lock immediately or not
  // If the queue is empty then yea
  if (!list->empty()) {
    // If the queue only contains shared locks then we
    // can also get the lock right away
    for (TNode<LockRequest>* node = list->head; node != NULL; node = node->next) {
      const LockRequest ilr = node->data;
      if (ilr.mode_ != SHARED) {
        newreq.state_ = WAIT;
        break;
      } else {
      }
    }
  }

  newnode = list->insertRequest(newreq);
  return newnode;
}

TNode<LockRequest>* LatchedLockManager::ReadLock(Txn* txn, const Key key) {
  TNode<LockRequest>* newnode = TryReadLock(txn, key);
  while (newnode->data.state_ != ACTIVE) do_pause();
  return newnode;
}

void LatchedLockManager::Release(Txn* txn, const Key key) {

  LockRequestLinkedList * list = lock_table.get_list(key);
  TNode<LockRequest>* req = list->head;
  TNode<LockRequest>* toRelease;
  bool foundExcl = false;
  bool foundOurTxn = false;
  while (req != NULL) {
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
      {
        Pthread_mutex_guard guard(KeyMutex(key));
        list->deleteRequest(req);
      }
      if (foundExcl == true || req->data.mode_ == SHARED) {
        break;
      }
    } else if (req->data.mode_ == EXCLUSIVE) {
      foundExcl = true;
      if (foundOurTxn) {
        break;
      }
    } else if (foundOurTxn) {
      req->data.state_ = ACTIVE;
    }
    req = req->next;
  }
}

LockState LatchedLockManager::CheckState(const Txn* txn, const Key key) {
  Pthread_mutex_guard guard(KeyMutex(key));

  TNode<LockRequest> *node = lock_table.get_list(key)->head;
  for (; node != NULL; node = node->next) {
    if (node->data.txn_ == txn) {
      return node->data.state_;
    }
  }

  return NOT_FOUND;
}
