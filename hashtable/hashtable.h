#ifndef HASHTABLE_HASHTABLE_H
#define HASHTABLE_HASHTABLE_H

#include <pthread.h>
#include <new>

#include "LockRequestLinkedList.h"



#define DEFAULT_BUCKET_SIZE 5


struct Bucket {
  LockRequestLinkedList* slots[DEFAULT_BUCKET_SIZE];
  int keys[DEFAULT_BUCKET_SIZE];
};


class Hashtable {
public:
  Hashtable(int n);
  void lock_insert(int key, LockRequest& lr);
  TNode<LockRequest>* get_list(int key);
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

#endif
