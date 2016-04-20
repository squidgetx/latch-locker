#ifndef HASHTABLE_LOCKREQUESTLINKEDLIST_H
#define HASHTABLE_LOCKREQUESTLINKEDLIST_H

#include <pthread.h>
#include "TLinkedList.h"
#include "lock_pool.h"
#include "util/util.h"
#include "lock_request.h"

#define DEFAULT_LIST_SIZE 5

class LockRequestLinkedList: private TLinkedList<LockRequest>
{
public:
  LockRequestLinkedList(LockPool* lock_pool, int init_mem);
  void insertRequest(LockRequest lr);
  void deleteRequest(TNode<LockRequest>* lr);
  void atomic_lock_insert(LockRequest lr);
  TNode<LockRequest> * latch_free_next(TNode<LockRequest> * req);
  using TLinkedList<LockRequest>::head;
  using TLinkedList<LockRequest>::tail;
private:
  TNode<LockRequest> * createRequest(LockRequest lr);
  void restoreChunk(TNode<LockRequest>* lr);
  int size_to_req;
  TLinkedList<MemoryChunk<TNode<LockRequest> > >* memory_list;
  LockPool * lock_pool;
};

#endif
