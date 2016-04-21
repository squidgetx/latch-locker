#include "key_lock_manager.h"

KeyLockManager::KeyLockManager(int nbuckets) : hashtable(nbuckets) {
}

bool KeyLockManager::WriteLock(Txn* txn, const Key key) {
  hashtable.lock(key);

  LockRequestLinkedList * list = hashtable.get_list(key);

  // add the lock to the queue
  LockRequest * newreq = new LockRequest(EXCLUSIVE, txn);

  bool wasEmpty = list->empty();
  if (wasEmpty) {
    newreq->state_ = ACTIVE;
  } else {
    newreq->state_ = WAIT;
  }

  list->insertRequest(*newreq);

  hashtable.unlock(key);
  return wasEmpty;
}

bool KeyLockManager::ReadLock(Txn* txn, const Key key) {
  hashtable.lock(key);

  LockRequestLinkedList * list = hashtable.get_list(key);

  // add the lock to the queue
  LockRequest * newreq = new LockRequest(SHARED, txn);
  list->insertRequest(*newreq);

  newreq->state_ = WAIT;
  // See if we can get this lock immediately or not
  // If the queue is empty then yea
  if (!list->empty()) {
    // If the queue only contains shared locks then we
    // can also get the lock right away
    TNode<LockRequest> * node = list->head;
    for( ; node != NULL; node = node->next) {
      if (node->data.mode_ != SHARED) {
        newreq->state_ = ACTIVE;
        break;
      }
    }
  } else {
    newreq->state_ = ACTIVE;
  }
  hashtable.unlock(key);

  return (newreq->state_ == ACTIVE);
}


void KeyLockManager::Release(Txn* txn, const Key key) {
  hashtable.lock(key);

  LockRequestLinkedList * list = hashtable.get_list(key);
  if (list == NULL || list->empty()) {
    // don't bother releasing non existent lock
    hashtable.unlock(key);
    return;
  }

  // First find the txn in the lock request list
  TNode<LockRequest> * current;
  for(current = list->head; current != NULL; current = current->next) {
    if (current->data.txn_ == txn) {
      break;
    }
  }

  if (current == NULL) {
    // abort if the txn doesn't have a lock on this item
    hashtable.unlock(key);
    return;
  }

  int mode = current->data.mode_;
  TNode<LockRequest> * next = current->next;
  list->deleteRequest(current);

  if (next == NULL) {
    // nothing left to do if no other txns waiting on this lock
    hashtable.unlock(key);
    return;
  }

  if (mode == SHARED) {
    // don't do anything if you aren't the last shared lock holder
    if (next->data.mode_ == SHARED) {
      hashtable.unlock(key);
      return;
    }
  }

  // At this point, the released lock was either
  // a) an EXCLUSIVE lock or
  // b) a SHARED lock, AND the next txn is an EXCLUSIVE lock

  if (next->data.mode_ == SHARED) {
    // Grant the lock to all the locks in this segment waiting for
    // a SHARED lock
    for( ; next != NULL; next = next->next) {
      if (next->data.mode_ != SHARED) {
        break;
      }
      next->data.state_ = ACTIVE;
    }

  } else {
    // Just grant to the lock to this request (exclusive lock)
    next->state_ = ACTIVE;
  }

  hashtable.unlock(key);
}
