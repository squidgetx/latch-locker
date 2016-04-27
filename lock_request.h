#ifndef _LOCK_REQUEST_H_
#define _LOCK_REQUEST_H_

class Txn;
#include <pthread.h>


// This interface supports locks being held in both read/shared and
// write/exclusive modes.
enum LockMode {
  UNLOCKED = 0,
  SHARED = 1,
  EXCLUSIVE = 2,
};

enum LockState {
  ACTIVE = 0,
  WAIT = 1,
  OBSOLETE = 2,
  NOT_FOUND = 3
};

struct LockRequest {
  LockRequest(LockMode m, Txn* t) : txn_(t), mode_(m) {}
  Txn* txn_;       // Pointer to txn requesting the lock.
  LockMode mode_;  // Specifies whether this is a read or write lock request.
  volatile LockState state_;
};

#endif
