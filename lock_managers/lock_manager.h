// Interface for lock managers in the system.

#ifndef _LOCK_MANAGER_H_
#define _LOCK_MANAGER_H_

class Txn;

#include <pthread.h>
#include "util/common.h"
#include "hashtable/LockRequestLinkedList.h"
#include "hashtable/hashtable.h"
#include "lock_request.h"

// The number of buckets lock managers allocate in their hash tables.
class LockManager {
 public:
  LockManager(int nbuckets) : lock_table(nbuckets) {}
  virtual ~LockManager() {}

  // Attempts to grant a read lock to the specified transaction, enqueueing
  // request in lock table. Returns true if lock is immediately granted, else
  // returns false.
  //
  // Requires: Neither ReadLock nor WriteLock has previously been called with
  //           this txn and key.
  virtual bool ReadLock(Txn* txn, const Key key) = 0;

  // Attempts to grant a write lock to the specified transaction, enqueueing
  // request in lock table. Returns true if lock is immediately granted, else
  // returns false.
  //
  // Requires: Neither ReadLock nor WriteLock has previously been called with
  //           this txn and key.
  virtual bool WriteLock(Txn* txn, const Key key) = 0;

  // Releases lock held by 'txn' on 'key', or cancels any pending request for
  // a lock on 'key' by 'txn'. If 'txn' held an EXCLUSIVE lock on 'key' (or was
  // the sole holder of a SHARED lock on 'key'), then the next request(s) in the
  // request queue is granted.
  //
  // If the granted request(s) corresponds to a
  // transaction that has now acquired ALL of its locks, that transaction is
  // appended to the 'ready_txns_' queue.
  //
  // Note: At this point, Releasing a lock that is not held results in
  // *undefined* behavior.
  virtual void Release(Txn* txn, const Key key) = 0;

  // Sets '*owners' to contain the txn IDs of all txns holding the lock, and
  // returns the current LockMode of the lock: UNLOCKED if it is not currently
  // held, SHARED or EXCLUSIVE if it is, depending on the current state.
  // virtual LockMode Status(const Key key, vector<int>* owners) = 0;

  // Check the state of txn's lock.
  // @return NOT_FOUND if txn has no lock in the chain.
  virtual LockState CheckState(const Txn *txn, const Key key) = 0;

 protected:
  // List is a placeholder for linked list structure we will define later
  // also unordered map will be later implemented
  Hashtable lock_table;
};

#endif  // _LOCK_MANAGER_H_

