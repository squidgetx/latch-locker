#include "txn/txn.h"
#include "lock_managers/lock_manager.h"

void Txn::Execute(LockManager *lm) {
	// acquire phase
	for (int i = 0; i < keys.size(); i++) {
		std::pair<Key, LockMode> p = keys[i];

		Key k = p.first;
		LockMode mode = p.second;
		if (mode == SHARED) {
		  lm->ReadLock(this, k);
		} else if (mode == EXCLUSIVE) {
		  lm->WriteLock(this, k);
		}
	}

	// release phase
	for (int i = 0; i < keys.size(); i++) {
	  std::pair<Key, LockMode> p = keys[i];
	  Key k = p.first;
	  lm->Release(this, k);
	}
}
