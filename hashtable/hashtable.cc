#include <iostream>

#include "hashtable.h"
#include "util/common.h"

Hashtable::~Hashtable() {
    delete bucket_array;
    delete lock_array;
    delete list_array;
}

Hashtable::Hashtable(int n) {
  num_buckets = n;

  // Allocate space for the total number of linked lists we will need
  // (nbuckets * bucket size)
  list_array = reinterpret_cast<LockRequestLinkedList*> (new char[sizeof(LockRequestLinkedList)*num_buckets*DEFAULT_BUCKET_SIZE]);

  // Initialize bucket/lock arrays
  bucket_array = new Bucket[num_buckets];
  lock_array = new Pthread_mutex[num_buckets];

  // For every key initialize its LockRequestLinkedList object
  // as well the initial number of lock requests given to it
  //
  for (int i = 0; i < num_buckets*DEFAULT_BUCKET_SIZE; i++)
  {
    new (&list_array[i]) LockRequestLinkedList();
  }

  // For every bucket, initialize its lock and distribute the LockRequestLinkedList objects
  // to each bucket
  for (int i = 0; i < num_buckets; i++)
  {
    new (&lock_array[i]) Pthread_mutex();
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
      std::cerr << key << " not found in bucket " << b_index << "\n";
    }
  }
  return bucket_array[b_index].slots[i];
}

void Hashtable::lock(Key key) {
  // get the latch on the bucket
  int b_index = hash(key);
  lock_array[b_index].lock();
}

void Hashtable::unlock(Key key) {
  int b_index = hash(key);
  lock_array[b_index].unlock();
}

Pthread_mutex& Hashtable::mutex(const Key key) {
  int b_index = hash(key);
  return lock_array[b_index];
}

