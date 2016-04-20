#include <iostream>

#include "hashtable.h"
#include "lock_request.h"

int main()
{
	std::cout << "output should be\n31\n32\n32\n";
	std::cout << "beginning test." <<std::endl;
	Hashtable* hasher = new Hashtable(1000);
	LockRequest r1(SHARED, reinterpret_cast<Txn*>(31));
	LockRequest r2(EXCLUSIVE, reinterpret_cast<Txn*>(32));
	hasher->lock_insert(15,r1);
	hasher->lock_insert(15,r2);
	TNode<LockRequest>* it = hasher->latch_free_get_list(15)->head;
	while (it != NULL)
	{
		std::cout << it->data.txn_ << std::endl;
		it = it->next;
	}
	hasher->lock_delete(15, hasher->latch_free_get_list(15)->head);
	it = hasher->latch_free_get_list(15)->head;
	while (it != NULL)
	{
		std::cout << it->data.txn_ << std::endl;
		it = it->next;
	}
	std::cout << "done table" << std::endl;
}
