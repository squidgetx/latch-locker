#include "hashtable.h"

int main()
{
	std::cout << "output should be\n31\n32\n32\n";
	std::cout << "beginning test." <<std::endl;
	Hashtable* hasher = new Hashtable(1000);
	LockRequest r1(SHARED, 31);
	LockRequest r2(EXCLUSIVE, 32);
	hasher->lock_insert(15,r1);
	hasher->lock_insert(15,r2);
	TNode<LockRequest>* it = hasher->get_list(15);
	while (it != NULL)
	{
		std::cout << it->data.txn << std::endl;
		it = it->next;
	}
	hasher->lock_delete(15, hasher->get_list(15));
	it = hasher->get_list(15);
	while (it != NULL)
	{
		std::cout << it->data.txn << std::endl;
		it = it->next;
	}
	std::cout << "done table" << std::endl;
}
