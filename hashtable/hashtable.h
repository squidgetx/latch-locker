#ifndef HASHTABLE_HASHTABLE_H
#define HASHTABLE_HASHTABLE_H

#include <pthread.h>
#include <new>

#include "LockRequestLinkedList.h"
#include "util/common.h"
#include "util/mutex.h"

#define DEFAULT_BUCKET_SIZE 5

/**
 * A hash bucket for Hashtable.  Stores at most DEFAULT_BUCKET_SIZE hash table
 * values, using a hashing with chaining strategy.
 */
struct Bucket {
  LockRequestLinkedList* slots[DEFAULT_BUCKET_SIZE];
  int keys[DEFAULT_BUCKET_SIZE];
};

/**
 * A hash table for storing lock requests.  The hash table supports
 * granular locking.
 *
 * This lock table is backed by a preallocated region of memory, the size of
 * which is determined by the initial size and the number of items in each
 * bucket.
 */
class Hashtable {
public:
  /**
   * Initialize a hashtable, with @n buckets preallocated
   *
   * The hashtable will be able to store at most @n*<DEFAULT_BUCKET_SIZE>
   * keys.  If a bucket overflows, the result is undefined.
   */
  Hashtable(int n);

  /**
   * Insert a lock request @lr into the table for @key.
   */
 // void lock_insert(Key key, LockRequest& lr);

  /**
   * Get a reference to the lock request list for @key.
   */
  LockRequestLinkedList * get_list(Key key);

  /**
   * Lock the bucket corresponding to Key key
   */
  void lock(Key key);

  /**
   * Unlock the bucket corresponding to Key key
   */
  void unlock(Key key);

  Pthread_mutex& mutex(const Key key);

  /**
   * Remove a lock request @lr from @key
   */
 // void lock_delete(Key key, TNode<LockRequest>* lr);

  /**
   * Get the mutex protecting a particular key
   */
 // pthread_mutex_t* get_mutex(Key key);
private:
  inline int hash(Key key);

  int num_buckets;

  // Array of buckets (one bucket contains multiple linked lists
  Bucket* bucket_array;

  // Memory pool of linked list pointers
  LockRequestLinkedList* list_array;

  // Bucket locks.
  Pthread_mutex* lock_array;

  // Lock Pool
  LockPool * lock_pool;
};

#endif
