#include <vector>
#include <utility>

#include "../lock_managers/lock_manager.h"
#include "../util/common.h"
#include "../txn.h"

// redefined, but with key
struct LockRequest {
  LockRequest(LockMode m, Txn* t, Key k) : txn_(t), mode_(m), key_(k) {}
  Txn* txn_;       // Pointer to txn requesting the lock.
  Key key_;
  LockMode mode_;  // Specifies whether this is a read or write lock request.
  LockState state_;
};

class Tester {
 public:
  Tester();
  void Run();


 private:
 	// one LockRequest Sequence
 	void Benchmark(std::vector<LockRequest> lock_requests);
 	std::vector<LockRequest> GenerateRequests(int n, int k, double w);

 	LockManager mgrs[3];

};