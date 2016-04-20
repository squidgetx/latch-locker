#include "tester.h"

#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <iterator>
#include <random>

#include "lock_managers/global_lock_manager.h"
#include "util/common.h"
#include "txn.h"
#include "lock_request.h"

static inline double GetTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec/1e6;
}

Tester::Tester() {
}

// run different tests
void Tester::Run() {
  std::vector<std::pair<Key, LockRequest>> lock_requests;

  int n = 10000; // number of lock requests
  int k = 10; // number of distinct keys
  double w = 0.5; // percentage of write locks

  lock_requests = GenerateRequests(n, k, w);
  Benchmark(lock_requests);
  lock_requests.clear();

}

std::vector<std::pair<Key, LockRequest>> Tester::GenerateRequests(int n, int k, double w) {
  std::vector<std::pair<Key, LockRequest>> lock_requests;

  for (int i = 0; i < n; i++) {
    Txn txn = Txn();
    Key key = 1 + (rand() % (int)k);
    LockMode mode = (((double) rand() / (RAND_MAX)) <= w) ? EXCLUSIVE : SHARED;
    LockRequest r = LockRequest(mode, &txn);
    lock_requests.push_back(std::make_pair(key, r));
  }

  return lock_requests;
}

void Tester::Benchmark(std::vector<std::pair<Key, LockRequest> > lock_requests) {
  LockManager *lm;
  // three types of mgr_s
  for (int i = 0; i < 1; i++) {
    switch(i) {
      case 0:
        lm = new GlobalLockManager();
        break;
      //case 1:
        //lm = new LatchFreeLockManager();
        //break;
      //case 2:
        //lm = new LatchFreeLockManager();
        //break;
      default:
        break;
    }
    double durations[2];

    // Record start time.
    double start = GetTime();

    // acquire phase
    for (auto it = lock_requests.begin(); it != lock_requests.end(); ++it) {
      Key k = it->first;
      LockRequest r = it->second;
      if (r.mode_ == SHARED) {
        lm->ReadLock(r.txn_, k);
      } else if (r.mode_ == EXCLUSIVE) {
        lm->WriteLock(r.txn_, k);
      }
    }

    // Record end time
    double end = GetTime();
    durations[0] = lock_requests.size() / (end-start);


    start = GetTime();
    // release phase
    for (auto it = lock_requests.begin(); it != lock_requests.end(); ++it) {
      Key k = it->first;
      LockRequest r = it->second;
      lm->Release(r.txn_, k);
    }

    end = GetTime();

    durations[1] = lock_requests.size() / (end-start);

    std::cout << "Lock Manager " << i << ". Avg Acquire Duration: " << durations[0]
                                      << ". Avg Release Duration: " << durations[1] << std::endl;
  }
}
