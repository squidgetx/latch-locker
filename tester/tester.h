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
  Tester();
  void Run();
private:
  // one LockRequest Sequence
  void Benchmark(std::vector<Txn*> * transactions);
  Txn *GenerateTransaction(int n, int k, double w);

  pthread_t pthreads[4];
  int NUM_THREADS = 4;
  int txn_counter = 0;


  int TRANSACTIONS_PER_TEST = 100000;
  int REQUESTS_PER_TRANSACTION = 20;
  int KEYS = 10;
};

#endif // TESTER_TESTER_H
