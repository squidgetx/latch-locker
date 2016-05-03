#include "txn/txn.h"
#include "lock_managers/lock_manager.h"
#include <new>
#include <cassert>

void Txn::Execute(LockManager *lm, int id, TNode<LockRequest>* lock_requests) {
  // acquire phase
  //std::cout << "executing txn " << txn_id << std::endl;
  for (int i = 0; i < keys.size(); i++) {
    std::pair<Key, LockMode> p = keys[i];

    Key k = p.first;
    LockMode mode = p.second;
    new (&lock_requests[id*REQUESTS_PER_TRANSACTION+i]) TNode<LockRequest>(LockRequest(mode,this));
    if (mode == SHARED) {
      lm->ReadLock(&lock_requests[id*REQUESTS_PER_TRANSACTION+i], k);
    } else if (mode == EXCLUSIVE) {
      lm->WriteLock(&lock_requests[id*REQUESTS_PER_TRANSACTION+i], k);
    }
  }

  for (int i = 0; i < spintime; i++) {
    single_work();
  }

  // release phase
  for (int i = 0; i < keys.size(); i++) {
    std::pair<Key, LockMode> p = keys[i];
    Key k = p.first;
   // std::cout << "releasing key " << k << std::endl;
    lm->Release(&lock_requests[id*REQUESTS_PER_TRANSACTION+i], k);
  }
}
