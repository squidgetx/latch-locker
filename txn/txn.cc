#include "txn/txn.h"
#include "lock_managers/lock_manager.h"

void Txn::Execute(LockManager *lm) {
	// acquire phase
	while (lr_queue.size()) {
		std::pair<Key, LockRequest> p = lr_queue.front();
		lr_queue.pop();

		Key k = p.first;
		LockRequest r = p.second;
		if (r.mode_ == SHARED) {
		  lm->ReadLock(r.txn_, k);
		} else if (r.mode_ == EXCLUSIVE) {
		  lm->WriteLock(r.txn_, k);
		}
	}

	// release phase
	while (lr_queue.size()) {
	  std::pair<Key, LockRequest> p = lr_queue.front();
	  lr_queue.pop();
	  Key k = p.first;
	  LockRequest r = p.second;
	  lm->Release(r.txn_, k);
	}
}
