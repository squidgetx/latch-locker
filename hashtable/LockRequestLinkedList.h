#include <pthread.h>
#include "TLinkedList.h"
#define DEFAULT_LIST_SIZE 5

enum LockMode {
  UNLOCKED = 0,
  SHARED = 1,
  EXCLUSIVE = 2,
};

struct LockRequest {
	LockRequest(LockMode m, int t) : txn(t), mode(m) {}
	int txn;       // Pointer to txn requesting the lock.
	LockMode mode;  // Specifies whether this is a read or write lock request.
};

class LockRequestLinkedList: private TLinkedList<LockRequest>
{
	public:
		LockRequestLinkedList(MemoryChunk<TNode<LockRequest> >* init_mem, TNode<LockRequest>* global_array, TNode<LockRequest>** global_array_ptr, pthread_mutex_t* global_lock);
		void insertRequest(LockRequest lr);
		void deleteRequest(TNode<LockRequest>* lr);
		using TLinkedList<LockRequest>::head;
		using TLinkedList<LockRequest>::tail;

	private:
		int size_to_req;
		TLinkedList<MemoryChunk<TNode<LockRequest> > >* memory_list;
		TNode<LockRequest>* global_array;
		TNode<LockRequest>** global_array_ptr;
		pthread_mutex_t* global_lock;

};
