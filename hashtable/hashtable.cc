#include <iostream>

#include "hashtable.h"
#include "util/common.h"

Hashtable::Hashtable(int n) {
  num_buckets = n;

  // Allocate space for the total number of linked lists we will need
  // (nbuckets * bucket size)
  list_array = reinterpret_cast<LockRequestLinkedList*> (new char[sizeof(LockRequestLinkedList)*num_buckets*DEFAULT_BUCKET_SIZE]);

  // Initialize pool of locks
  lock_pool = new LockPool(num_buckets * DEFAULT_BUCKET_SIZE * DEFAULT_LIST_SIZE);


  // Initialize bucket/lock arrays
  bucket_array = new Bucket[num_buckets];
  lock_array = new pthread_mutex_t[num_buckets];

  // For every key initialize its LockRequestLinkedList object
  // as well the initial number of lock requests given to it
  //
  for (int i = 0; i < num_buckets*DEFAULT_BUCKET_SIZE; i++)
  {
    new (&list_array[i]) LockRequestLinkedList(lock_pool, DEFAULT_LIST_SIZE);
  }

  // For every bucket, initialize its lock and distribute the LockRequestLinkedList objects
  // to each bucket
  for (int i = 0; i < num_buckets; i++)
  {
    pthread_mutex_init(&lock_array[i], NULL);
    for (int j = 0; j < DEFAULT_BUCKET_SIZE; j++)
    {
      bucket_array[i].slots[j] = &list_array[i*DEFAULT_BUCKET_SIZE + j];
      bucket_array[i].keys[j] = -1;
    }
  }
  // distribute keys
  for(int i = 0 ; i < num_buckets * DEFAULT_BUCKET_SIZE; i++) {
    for(int j = 0; j < DEFAULT_BUCKET_SIZE; j++) {
      if (bucket_array[hash(i)].keys[j] == -1) {
        //std::cout << "placing key " << i << "in bucket " << hash(i) << "\n";
        bucket_array[hash(i)].keys[j] = i;
        break;
      }
    }
  }
}

int Hashtable::hash(Key key) {
  return key % num_buckets;
}

/*
void Hashtable::lock_insert(Key key, LockRequest& lr) {
  int b_index = hash(key);
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
}*/

LockRequestLinkedList * Hashtable::get_list(Key key) {
  int b_index = hash(key);
  int i;
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
  return bucket_array[b_index].slots[i];
}

void Hashtable::lock(Key key) {
  // get the latch on the bucket
  int b_index = key % num_buckets;
  pthread_mutex_lock(&lock_array[b_index]);
}

void Hashtable::unlock(Key key) {
  int b_index = key % num_buckets;
  pthread_mutex_unlock(&lock_array[b_index]);
}
/*
void Hashtable::lock_delete(Key key, TNode<LockRequest>* lr)
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

pthread_mutex_t* Hashtable::get_mutex(Key key)
{
  int b_index = key % num_buckets;
  int i;
  return &lock_array[b_index];
}
*/






