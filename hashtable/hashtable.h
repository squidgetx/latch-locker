#include <pthread.h>
#include "LockRequestLinkedList.h"
#include <new>
#include <iostream>



#define DEFAULT_BUCKET_SIZE 5


struct Bucket {
	LockRequestLinkedList* slots[DEFAULT_BUCKET_SIZE];
	int* keys;
};


class Hashtable {
 public:

 	Hashtable(int n);
	void lock_insert(int key, LockRequest& lr);
	LockRequestLinkedList* get_list(int key);
	void lock_delete(int key, TNode<LockRequest>* lr);
 	pthread_mutex_t* get_mutex(int key);

 		
 private:
 	int num_buckets;
 	LockRequestLinkedList* list_array;
	TNode<LockRequest>* memory_array;
	TNode<LockRequest>* memory_ptr;

	
 	pthread_mutex_t* lock_array;
	pthread_mutex_t global_lock;
	Bucket* bucket_array;
};
