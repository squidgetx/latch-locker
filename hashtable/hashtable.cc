#include "hashtable.h"
#include <iostream>

Hashtable::Hashtable(int n) {
  num_buckets = n;

  list_array = reinterpret_cast<LockRequestLinkedList*> (new char[sizeof(LockRequestLinkedList)*num_buckets*DEFAULT_BUCKET_SIZE]);
  memory_array = reinterpret_cast<TNode<LockRequest>*> (new char[sizeof(TNode<LockRequest>)*num_buckets*DEFAULT_BUCKET_SIZE*DEFAULT_LIST_SIZE]);
  memory_ptr = memory_array;
  pthread_mutex_init(&global_lock, NULL);
  bucket_array = new Bucket[num_buckets];
  lock_array = new pthread_mutex_t[num_buckets];



  for (int i = 0; i < num_buckets*DEFAULT_BUCKET_SIZE; i++)
  {
    MemoryChunk<TNode<LockRequest> > mem(&memory_array[DEFAULT_LIST_SIZE*i], DEFAULT_LIST_SIZE);
    new (&list_array[i]) LockRequestLinkedList(&mem, memory_array, &memory_ptr, &global_lock);

  }

  for (int i = 0; i < num_buckets; i++)
  {
    pthread_mutex_init(&lock_array[i], NULL);
    for (int j = 0; j < DEFAULT_BUCKET_SIZE; j++)
    {
      bucket_array[i].slots[j] = &list_array[i*DEFAULT_BUCKET_SIZE + j];
      bucket_array[i].keys[j] = -1;
    }
  }
}

void Hashtable::lock_insert(int key, LockRequest& lr) {
  int b_index = key % num_buckets;
  int i;
  pthread_mutex_lock(&lock_array[b_index]);
  try {
    for (i = 0; i < DEFAULT_BUCKET_SIZE; i++) {
      if (bucket_array[b_index].keys[i] == -1) {
        bucket_array[b_index].keys[i] = key;
        break;
      }
      else if (bucket_array[b_index].keys[i] == key) break;
      else if (i == DEFAULT_BUCKET_SIZE - 1) throw 1;
    }
  }
  catch (int e) {
    if (e == 1) {
      std::cout << "out of space in bucket " << b_index << "\n";
    }
  }
  bucket_array[b_index].slots[i]->insertRequest(lr);
  pthread_mutex_unlock(&lock_array[b_index]);
}

TNode<LockRequest>* Hashtable::get_list(int key) {
  int b_index = key % num_buckets;
  int i;
  pthread_mutex_lock(&lock_array[b_index]);
  try {
    for (i = 0; i < DEFAULT_BUCKET_SIZE; i++) {
      if (bucket_array[b_index].keys[i] == key) break;
      else if (i == DEFAULT_BUCKET_SIZE - 1) throw 1;
    }
  }
  catch (int e) {
    if (e == 1) {
      std::cout << "not found in bucket " << b_index << "\n";
    }
  }
  pthread_mutex_unlock(&lock_array[b_index]);
  return bucket_array[b_index].slots[i]->head;
}

void Hashtable::lock_delete(int key, TNode<LockRequest>* lr)
{
  int b_index = key % num_buckets;
  int i;
  pthread_mutex_lock(&lock_array[b_index]);
  try {
    for (i = 0; i < DEFAULT_BUCKET_SIZE; i++) {
      if (bucket_array[b_index].keys[i] == key) break;
      else if (i == DEFAULT_BUCKET_SIZE - 1) throw 1;
    }
  }
  catch (int e) {
    if (e == 1) {
      std::cout << "not found in bucket " << b_index << "\n";
    }
  }
  bucket_array[b_index].slots[i]->deleteRequest(lr);
  pthread_mutex_unlock(&lock_array[b_index]);
}

pthread_mutex_t* Hashtable::get_mutex(int key)
{
  int b_index = key % num_buckets;
  int i;
  return &lock_array[b_index];
}






