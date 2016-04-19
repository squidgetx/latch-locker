#include "LockRequestLinkedList.h"

LockRequestLinkedList::LockRequestLinkedList(MemoryChunk<TNode<LockRequest> >* init_mem,
    TNode<LockRequest>* global_array,
    TNode<LockRequest>** global_array_ptr,
    pthread_mutex_t* global_lock) {

  this->global_array = global_array;
  this->global_lock = global_lock;
  this->global_array_ptr = global_array_ptr;
  size_to_req = 2*DEFAULT_LIST_SIZE;

  memory_list = new TLinkedList<MemoryChunk<TNode<LockRequest> > >; 
  TNode<MemoryChunk<TNode<LockRequest> > >* init_chunk = new TNode<MemoryChunk<TNode<LockRequest> > >(*init_mem);
  memory_list->append(init_chunk);
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
  uint64_t old_tail = xchgq((uint64_t *) &tail, (uint64_t) lock);
  if (old_tail != NULL) {
    ((TNode<LockRequest> *) old_tail)->next = lock;
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
    pthread_mutex_lock(global_lock);
    MemoryChunk<TNode<LockRequest> > new_chunk(*global_array_ptr,size_to_req);
    *global_array_ptr += size_to_req;
    pthread_mutex_unlock(global_lock);
    TNode<MemoryChunk<TNode<LockRequest> > >* new_node = new TNode<MemoryChunk<TNode<LockRequest> > >(new_chunk);
    memory_list->append(new_node);
  }

  TNode<LockRequest> * node = memory_list->head->data.loc;
  memory_list->head->data.size--;
  if (memory_list->head->data.size == 0)
  {
    memory_list->remove(memory_list->head);
  }
  memory_list->head->data.loc+=1;
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

