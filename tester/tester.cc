#include <iostream>
#include <cstdlib>
#include <iterator>
#include <random>

#include "tester.h"

Tester::Tester() {
  global_lock_manager = new GlobalLockManager();
  latchfree_lock_manager = new LatchFreeLockManager();
}

// run different tests
void Tester::Run() {
  std::vector<LockRequest> lock_requests;

  int n = 10000; // number of lock requests
  int k = 10; // number of distinct keys
  double w = 0.5; // percentage of write locks

  lock_requests = GenerateRequests(n, k, w);
  Benchmark(lock_requests);
  lock_requests.clear();

}

std::vector<LockRequest> Tester::GenerateRequests(int n, int k, double w) {
  std::vector<LockRequest> lock_requests;

  for (int i = 0; i < n; i++) {
    Txn txn = Txn();
    Key key = 1 + (rand() % (int)k);
    LockMode mode = (((double) rand() / (RAND_MAX)) <= w) ? EXCLUSIVE : SHARED;   
    LockRequest r = LockRequest(mode, &txn);
    r.key_ = key;
    lock_requests.push_back(r);
  }

  return lock_requests;
}

void Tester::Benchmark(std::vector<LockRequest> lock_requests) {
  LockManager *lm;
  // three types of mgr_s
  for (int i = 0; i < 3; i++) {
    switch(i) {
      case 0:
        lm = new GlobalLockManager();
        break;
      case 1:
        lm = new LatchFreeLockManager();
        break;
      case 2:
        lm = new LatchFreeLockManager();
        break;
      default:
      break;
    }
    double durations[2];

    // Record start time.
    double start = GetTime();

    // acquire phase
    for(std::vector<LockRequest>::iterator it = lock_requests.begin(); it != lock_requests.end(); ++it) {
      LockRequest r = *it;
      if (r.mode_ == SHARED) {
        lm->ReadLock(r.txn_, r.key_);
      } else if (r.mode_ == EXCLUSIVE) {
        lm->WriteLock(r.txn_, r.key_);
      }
    }

    // Record end time
    double end = GetTime();
    durations[0] = lock_requests.size() / (end-start);


    start = GetTime();
    // release phase
    for(std::vector<LockRequest>::iterator it = lock_requests.begin(); it != lock_requests.end(); ++it) {
      LockRequest r = *it;
      lm->Release(r.txn_, r.key_);
    }
    end = GetTime();

    durations[1] = lock_requests.size() / (end-start);

    std::cout << "Lock Manager " << i << ". Avg Acquire Duration: " << durations[0] 
                                      << ". Avg Release Duration: " << durations[1] << std::endl; 
  }
}
