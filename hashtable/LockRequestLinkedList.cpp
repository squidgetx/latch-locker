#include "LockRequestLinkedList.h"
LockRequestLinkedList::LockRequestLinkedList(MemoryChunk<TNode<LockRequest> >* init_mem, TNode<LockRequest>* global_array, TNode<LockRequest>** global_array_ptr, pthread_mutex_t* global_lock) {
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
	if (memory_list->head == NULL)
	{
		pthread_mutex_lock(global_lock);
		MemoryChunk<TNode<LockRequest> > new_chunk(*global_array_ptr,size_to_req);
		*global_array_ptr += size_to_req;
		pthread_mutex_unlock(global_lock);
		TNode<MemoryChunk<TNode<LockRequest> > >* new_node = new TNode<MemoryChunk<TNode<LockRequest> > >(new_chunk);
		memory_list->append(new_node);
	}
	TNode<LockRequest>* node_mem = memory_list->head->data.loc;
	memory_list->head->data.size--;
	if (memory_list->head->data.size == 0)
	{
		memory_list->remove(memory_list->head);
	}
	memory_list->head->data.loc+=1;
	node_mem->data.txn = lr.txn;
	node_mem->data.mode = lr.mode;
	append(node_mem);
}

void LockRequestLinkedList::deleteRequest(TNode<LockRequest>* lr)
{
	MemoryChunk<TNode<LockRequest> > new_chunk(lr,1);
	TNode<MemoryChunk<TNode<LockRequest> > >* new_node = new TNode<MemoryChunk<TNode<LockRequest> > >(new_chunk);
	memory_list->append(new_node);
	remove(lr);

}