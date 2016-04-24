#ifndef _LOCK_POOL_H_
#define _LOCK_POOL_H_

#include "lock_request.h"
#include "hashtable/TLinkedList.h"
#include "hashtable/TNode.h"
#include <iostream>

class LockPool
{
public:
  LockPool(int n);
  void get_uninit_locks(int n, MemoryChunk<TNode<LockRequest> > &mem);

private:
  TNode<LockRequest>* memory_array;
  TNode<LockRequest>* memory_ptr;
  pthread_mutex_t pool_mutex;
  int capacity;
};


#endif
