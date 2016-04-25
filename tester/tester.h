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

  std::queue<std::pair<Key, LockRequest>> lr_queue;
  pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_t pthreads[4];
  int NUM_THREADS=4;
};

#endif // TESTER_TESTER_H
