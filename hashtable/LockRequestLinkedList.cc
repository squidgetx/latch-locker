#include <cstdint> 
#include "LockRequestLinkedList.h"
#include <cassert>

LockRequestLinkedList::LockRequestLinkedList(LockPool * lock_pool, int init_mem) {
  // Construct the lockrequestlinkedlist with some initial amount of memory
  // with a reference to the global lock pool for backup mem
  this->lock_pool = lock_pool;
  memory_list = new TLinkedList<MemoryChunk<TNode<LockRequest> > >();
  lock_pool->get_uninit_locks(init_mem, memory_list);
  size_to_req = 2*init_mem;
}

TNode<LockRequest>* LockRequestLinkedList::insertRequest(LockRequest lr)
{
  // Create and append the new lock request. Assumes exclusive/atomic
  // access to the list (caller is responsible for this)
  TNode<LockRequest> * node_mem = createRequest(lr);
  append(node_mem);
  return node_mem;
}

void LockRequestLinkedList::deleteRequest(TNode<LockRequest>* lr)
{
  // Remove lock request from the list. Assumes exclusive/atmoic
  // access to the list (caller is responsible for this)
  restoreChunk(lr);
  remove(lr);
}

void LockRequestLinkedList::atomic_synchronize() {
    barrier();
}

void LockRequestLinkedList::next_pointer_update() {
    TNode<LockRequest>* old_head = this->head;
    //update head to first non-obsolete lock
    while (old_head != NULL && old_head->data.fstate_.state == OBSOLETE) {
        if (!cmp_and_swap((uint64_t*)&(this->head), (uint64_t) old_head, (uint64_t)old_head->next))
            break; //another thread is updating, so let it do its thing
        else
            old_head = this->head;
    }
    TNode<LockRequest>* prev = this->head;
    if (prev == NULL) return;
    TNode<LockRequest>* node = prev->next;
    while (node != NULL) {
        if (node->data.fstate_.state == OBSOLETE) {
            if (!cmp_and_swap((uint64_t*)&(prev->next), (uint64_t) node, (uint64_t)node->next))
                break; //another thread is updating, so let it do its thing
        }
        prev = node;
        node = node->next;
    }
}

TNode<LockRequest> * LockRequestLinkedList::atomic_lock_insert(LockRequest lr)
{
  // Append the new lock request using the latch free algorithm
  //std::cout << "Creating request\n";
  TNode<LockRequest> * lock = atomicCreateRequest(lr);
  //TNode<LockRequest> * lock = new TNode<LockRequest>(lr);
  //std::cout << "Appending request\n";
  atomic_append(lock);
  atomic_synchronize();
 // std::cout << "Updating next pointers\n";
  next_pointer_update();
  atomic_synchronize();
  return lock;
}

TNode<LockRequest> * LockRequestLinkedList::latch_free_next(TNode<LockRequest>* req)
{
  while (req->next == NULL && req != tail) {
    atomic_synchronize();
  }
  return req->next;
}

TNode<LockRequest> * LockRequestLinkedList::atomicCreateRequest(LockRequest lr) {
  // Get the TNode<LockRequest> from the memory pool associated with this
  // lock request linked list.

  // If the local memory pool is empty, get more from the global pool
  int64_t rem_size;
  TNode<MemoryChunk<TNode<LockRequest> > >* memhead;
  do {
      memhead = memory_list->head;
      while (memhead->data.size <= 0)
      {
        assert(memhead->data.loc != NULL);
          if (memhead->next != NULL) cmp_and_swap((uint64_t*)&(memory_list->head), (uint64_t) memhead, (uint64_t) memhead->next); 
          else {
              if (cmp_and_swap((uint64_t*)&allocating, 0, 1)) {
                  lock_pool->get_uninit_locks(size_to_req, memory_list); 
                  size_to_req *= 2;
              }
          }

          memhead = memory_list->head;
      }

     rem_size = (int64_t)fetch_and_decrement((uint64_t*)&(memhead->data.size));
  } while (rem_size < 0);

  if (rem_size == 0) allocating = 0;

  TNode<LockRequest> * node = ((TNode<LockRequest>*) fetch_and_add((uint64_t*) &(memhead->data.loc), sizeof(TNode<LockRequest>))) - 1;
  assert(memhead->data.loc != NULL);
  node->data.txn_ = lr.txn_;
  node->data.mode_ = lr.mode_;

  return node;
}

TNode<LockRequest> * LockRequestLinkedList::createRequest(LockRequest lr) {
  // Get the TNode<LockRequest> from the memory pool associated with this
  // lock request linked list.

  // If the local memory pool is empty, get more from the global pool
  if (memory_list->head->data.size == 0) {
   // std::cout << "local pool empty, requesting " << size_to_req << "\n";
     lock_pool->get_uninit_locks(size_to_req, memory_list);
     size_to_req *= 2;
  } else {
   // std::cout << "local pool head node has " << memory_list->head->data.size << " blocks free!\n";

  }

  TNode<LockRequest> * node = memory_list->head->data.loc;
  memory_list->head->data.size--;
  memory_list->head->data.loc++;
  node->data = lr;

  return node;
}

void LockRequestLinkedList::restoreChunk(TNode<LockRequest>* lr) {
  // Restore the chunk of memory associated with the given lock request
  // to the local memory pool
  MemoryChunk<TNode<LockRequest> > new_chunk(lr,1);
  TNode<MemoryChunk<TNode<LockRequest> > >* new_node = new TNode<MemoryChunk<TNode<LockRequest> > >(new_chunk);
  memory_list->append(new_node);
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
    if (lr.fstate_.state == WAIT) {
      std::cout << "WAIT ";
    } else if (lr.fstate_.state == ACTIVE) {
      std::cout << "ACTIVE ";
    } else if (lr.fstate_.state == OBSOLETE) {
      std::cout << "OBSOLETE ";
    } 
  //  std::cout << lr.txn_;
    std::cout << std::endl;
  }
  std::cout << "   >\n";
}
