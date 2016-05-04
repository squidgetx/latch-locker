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
  Tester() : txn_counter(0), NUM_HOT_REQUESTS(20), KEYS(100000) {}
  void Run();
private:
  // one LockRequest Sequence
  void Benchmark(std::vector<Txn*> * transactions);
  Txn *GenerateTransaction(int n, double w, std::vector<Key> &hot_set, std::vector<Key> &cold_set);

  pthread_t pthreads[600];
  int NUM_THREADS;
  int txn_counter;


  int NUM_HOT_REQUESTS;

  int KEYS;
};

#endif // TESTER_TESTER_H
