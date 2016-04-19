#include <pthread.h>
#include "TLinkedList.h"

#include "../lock_request.h"

#define DEFAULT_LIST_SIZE 5

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
