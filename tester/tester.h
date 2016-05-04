#ifndef TESTER_TESTER_H
#define TESTER_TESTER_H

#include <vector>
#include <utility>
#include <pthread.h>
#include <queue>

#include "lock_managers/global_lock_manager.h"
#include "lock_managers/key_lock_manager.h"
#include "lock_managers/latch_free_lock_manager.h"

#include "util/common.h"
#include "txn/txn.h"
#include "lock_request.h"

class Tester {
public:
  Tester() {}
  void Run();
private:
  // one LockRequest Sequence
  void Benchmark(std::vector<Txn*> * transactions, int nthreads);
  Txn *GenerateTransaction(double w, std::vector<Key> &hot_set, std::vector<Key> &cold_set);

  pthread_t pthreads[600];
  int txn_counter;

};

#endif // TESTER_TESTER_H
