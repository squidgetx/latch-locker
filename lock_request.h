#ifndef _LOCK_REQUEST_H_
#define _LOCK_REQUEST_H_
#include <cstdint>
class Txn;

//LockState defines
#define WAIT 0
#define ACTIVE 1
#define OBSOLETE 2
#define NOT_FOUND 3

const uint64_t ACTIVE_NONFIRST = ((uint64_t)ACTIVE << 32);
const uint64_t OBSOLETE_NONFIRST = ((uint64_t)OBSOLETE << 32);
const uint64_t ACTIVE_FIRST = ((uint64_t)ACTIVE << 32) | 1;
const uint64_t WAIT_NONFIRST = ((uint64_t)WAIT << 32);
const uint64_t WAIT_FIRST = ((uint64_t)WAIT << 32) | 1;


// This interface supports locks being held in both read/shared and
// write/exclusive modes.
enum LockMode {
  UNLOCKED = 0,
  SHARED = 1,
  EXCLUSIVE = 2,
};

union LockState {
  uint64_t full;
  struct {
    uint32_t state;
    uint32_t firstholder;
  };
};

struct LockRequest {
  LockRequest(LockMode m, Txn* t) : txn_(t), mode_(m) {}
  Txn* txn_;       // Pointer to txn requesting the lock.
  LockMode mode_;  // Specifies whether this is a read or write lock request.
  LockState fstate_ = {.full = 0};
};

#endif
