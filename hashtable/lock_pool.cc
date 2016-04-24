#include "lock_pool.h"
#include <string.h>

LockPool::LockPool(int n) : capacity(n) {
  // For now, we just use a giant array
  memory_array = reinterpret_cast<TNode<LockRequest>*> (new char[
      sizeof(TNode<LockRequest>)*n]);
  memset(memory_array, 0, sizeof(TNode<LockRequest>) * n);
  memory_ptr = memory_array;
  pthread_mutex_init(&pool_mutex, NULL);
}

void LockPool::get_uninit_locks(int n, TLinkedList<MemoryChunk<TNode<LockRequest> > >* mem_list) {
  // Get a chunk of n lock requests
  pthread_mutex_lock(&pool_mutex);
  if (capacity - n < 0) {
    std::cerr << "Global lock pool out of mem\n";
  }
  capacity -= n;
  TNode<LockRequest>* local_memory_ptr = memory_ptr;
  memory_ptr += n;
  pthread_mutex_unlock(&pool_mutex);

  mem_list->atomic_append(new TNode<MemoryChunk<TNode<LockRequest>>>(MemoryChunk<TNode<LockRequest>>(local_memory_ptr,n)));
}
