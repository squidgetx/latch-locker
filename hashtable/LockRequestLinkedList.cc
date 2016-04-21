#include <cstdint>

#include "LockRequestLinkedList.h"

LockRequestLinkedList::LockRequestLinkedList(LockPool * lock_pool, int init_mem) {
  // Construct the lockrequestlinkedlist with some initial amount of memory
  // with a reference to the global lock pool for backup mem
  this->lock_pool = lock_pool;
  memory_list = new TLinkedList<MemoryChunk<TNode<LockRequest> > >();
  MemoryChunk<TNode<LockRequest> > * init_chunk = lock_pool->get_uninit_locks(init_mem);
  TNode<MemoryChunk<TNode<LockRequest> > > * init_node = new TNode<MemoryChunk<TNode<LockRequest> > >(*init_chunk);
  size_to_req = 2*init_mem;
  memory_list->append(init_node);
}

void LockRequestLinkedList::insertRequest(LockRequest lr)
{
  // Create and append the new lock request. Assumes exclusive/atomic
  // access to the list (caller is responsible for this)
  TNode<LockRequest> * node_mem = createRequest(lr);
  append(node_mem);
}

void LockRequestLinkedList::deleteRequest(TNode<LockRequest>* lr)
{
  // Remove lock request from the list. Assumes exclusive/atmoic
  // access to the list (caller is responsible for this)
  restoreChunk(lr);
  remove(lr);
}

void atomic_synchronize() {

}

void next_pointer_update() {

}

void LockRequestLinkedList::atomic_lock_insert(LockRequest lr)
{
  // Append the new lock request using the latch free algorithm
  TNode<LockRequest> * lock = createRequest(lr);
  TNode<LockRequest> * old_tail = (TNode<LockRequest> *)
    xchgq((uint64_t *) &tail, (uint64_t) lock);
  if (old_tail != NULL) {
    old_tail->next = lock;
  }
  atomic_synchronize();
  next_pointer_update();
  atomic_synchronize();
}

TNode<LockRequest> * LockRequestLinkedList::latch_free_next(TNode<LockRequest>* req)
{
  while (req->next == NULL && req != tail) {
    atomic_synchronize();
  }
  return req->next;
}

TNode<LockRequest> * LockRequestLinkedList::createRequest(LockRequest lr) {
  // Get the TNode<LockRequest> from the memory pool associated with this
  // lock request linked list.

  // If the local memory pool is empty, get more from the global pool
  if (memory_list->head == NULL)
  {
   // std::cout << "local pool empty, requesting " << size_to_req << "\n";
    TNode<MemoryChunk<TNode<LockRequest> > >* new_node = new TNode<MemoryChunk<TNode<LockRequest> > >(*lock_pool->get_uninit_locks(size_to_req));
    memory_list->append(new_node);
  } else {
   // std::cout << "local pool head node has " << memory_list->head->data.size << " blocks free!\n";

  }

  TNode<LockRequest> * node = memory_list->head->data.loc;
  memory_list->head->data.size--;
  memory_list->head->data.loc+=1;
  if (memory_list->head->data.size == 0)
  {
    memory_list->remove(memory_list->head);
  }
  node->data.txn_ = lr.txn_;
  node->data.mode_ = lr.mode_;
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
      std::cout << "EXCLUSIVE\n";
    } else {
      std::cout << "SHARED\n";
    }
  }
  std::cout << "   >\n";
}
