#ifndef TESTER_TESTER_H
#define TESTER_TESTER_H

#include <vector>
#include <utility>

#include "lock_managers/global_lock_manager.h"
#include "util/common.h"
#include "txn.h"
#include "lock_request.h"

class Tester {
public:
  Tester();
  void Run();
private:
  // one LockRequest Sequence
  void Benchmark(std::vector<std::pair<Key, LockRequest>> lock_requests);
  std::vector<std::pair<Key, LockRequest>> GenerateRequests(int n, int k, double w);
};

#endif // TESTER_TESTER_H
