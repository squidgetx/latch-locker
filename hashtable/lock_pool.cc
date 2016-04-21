#include "lock_pool.h"

LockPool::LockPool(int n) {
  // For now, we just use a giant array
  memory_array = reinterpret_cast<TNode<LockRequest>*> (new char[
      sizeof(TNode<LockRequest>)*n]);
  memory_ptr = memory_array;
  pthread_mutex_init(&pool_mutex, NULL);
}

MemoryChunk<TNode<LockRequest> > * LockPool::get_uninit_locks(int n) {
  // Get a chunk of n lock requests
  pthread_mutex_lock(&pool_mutex);
  MemoryChunk<TNode<LockRequest> > * mem = new MemoryChunk<TNode<LockRequest> >(memory_ptr, n);
  memory_ptr += n;
  pthread_mutex_unlock(&pool_mutex);
  return mem;
}
