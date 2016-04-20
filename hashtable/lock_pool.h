#ifndef _LOCK_POOL_H_
#define _LOCK_POOL_H_

#include "lock_request.h"
#include "hashtable/TLinkedList.h"
#include "hashtable/TNode.h"

class LockPool
{
  public:
    LockPool(int n);
    TNode<LockRequest>* get_uninit_lock();
    MemoryChunk<TNode<LockRequest> > * get_uninit_locks(int n);

  private:

    TNode<LockRequest>* memory_array;
    TNode<LockRequest>* memory_ptr;
    pthread_mutex_t pool_mutex;
    
};


#endif
