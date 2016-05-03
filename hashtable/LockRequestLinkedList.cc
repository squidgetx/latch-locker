#include <cstdint> 
#include "LockRequestLinkedList.h"
#include <cassert>

TNode<LockRequest>* LockRequestLinkedList::insertRequest(TNode<LockRequest> *lr)
{
  // Create and append the new lock request. Assumes exclusive/atomic
  // access to the list (caller is responsible for this)
  append(lr);
  return lr;
}

void LockRequestLinkedList::deleteRequest(TNode<LockRequest>* lr)
{
  // Remove lock request from the list. Assumes exclusive/atmoic
  // access to the list (caller is responsible for this)
  remove(lr);
}

void LockRequestLinkedList::atomic_synchronize() {
    barrier();
}

void LockRequestLinkedList::next_pointer_update() {
    TNode<LockRequest>* old_head = this->head;
    //update head to first non-obsolete lock
    while (old_head != NULL && old_head->data.state_ == OBSOLETE) {
        if (!cmp_and_swap((uint64_t*)&(this->head), (uint64_t) old_head, (uint64_t)old_head->next))
            break; //another thread is updating, so let it do its thing
        else
        {
            old_head = this->head;
        }
    }
    TNode<LockRequest>* prev = this->head;
    if (prev == NULL) 
      return;
    TNode<LockRequest>* node = prev->next;
    while (node != NULL && node != tail) {
        if (node->data.state_ == OBSOLETE) {
            if (!cmp_and_swap((uint64_t*)&(prev->next), (uint64_t) node, (uint64_t) node->next))
                break; //another thread is updating, so let it do its thing
        }
        prev = node;
        node = node->next;
    }
}

TNode<LockRequest> * LockRequestLinkedList::atomic_lock_insert(TNode<LockRequest> *lr)
{
  // Append the new lock request using the latch free algorithm

  atomic_synchronize();
  atomic_append(lr);
  atomic_synchronize();
  next_pointer_update();
  atomic_synchronize();

  return lr;
}

TNode<LockRequest> * LockRequestLinkedList::latch_free_next(TNode<LockRequest>* req)
{
  atomic_synchronize();
  while (req->next == NULL && req != tail) {
    atomic_synchronize();
  }
  return req->next;
}

void LockRequestLinkedList::printList() {
  // print the list for debugging purposes
  std::cout << "   <\n";
  for(TNode<LockRequest> * node = head; node != NULL; node = node->next) {
    LockRequest lr = node->data;
    std::cout << "     Txn " << lr.txn_->txn_id;
    if (lr.mode_ == EXCLUSIVE) {
      std::cout << " EXCLUSIVE ";
    } else {
      std::cout << " SHARED ";
    }
    if (lr.state_ == WAIT) {
      std::cout << "WAIT ";
    } else if (lr.state_ == ACTIVE) {
      std::cout << "ACTIVE ";
    } else if (lr.state_ == OBSOLETE) {
      std::cout << "OBSOLETE ";
    } 
  //  std::cout << lr.txn_;
    std::cout << std::endl;
  }
  std::cout << "   >\n";
}
