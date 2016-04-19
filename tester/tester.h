#include <vector>
#include <utility>

#include "lock_managers/lock_manager.h"
#include "util/common.h"
#include "txn.h"
#include "lock_request.h"

class Tester {
 public:
  Tester();
  void Run();


 private:
 	// one LockRequest Sequence
 	void Benchmark(std::vector<LockRequest> lock_requests);
 	std::vector<LockRequest> GenerateRequests(int n, int k, double w);


 	GlobalLockManager *global_lock_manager;
 	// LatchedLockManager *latched_lock_manager;
 	LatchFreeLockManager *latchfree_lock_manager;

};
