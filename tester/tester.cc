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

  int n = 100000; // number of lock requests
  int k; // number of distinct keys
  double w; // percentage of write locks

  k = 100;
  w = 0.0;
  std::cout << "high num of different keys, all shared locks " << std::endl;
  lock_requests = GenerateRequests(n, k, w);
  Benchmark(lock_requests);
  lock_requests.clear();
  std::cout << "======" << std::endl;

  k = 100;
  w = 1.0;
  std::cout << "high num of different keys, all exclusive locks " << std::endl;
  lock_requests = GenerateRequests(n, k, w);
  Benchmark(lock_requests);
  lock_requests.clear();
  std::cout << "======" << std::endl;

  k = 100;
  w = 0.5;
  std::cout << "high num of different keys, mixed locks 50/50 " << std::endl;
  lock_requests = GenerateRequests(n, k, w);
  Benchmark(lock_requests);
  lock_requests.clear();
  std::cout << "======" << std::endl;

  k = 5;
  w = 0.0;
  std::cout << "low num of different keys, all shared locks " << std::endl;
  lock_requests = GenerateRequests(n, k, w);
  Benchmark(lock_requests);
  lock_requests.clear();
  std::cout << "======" << std::endl;

  k = 5;
  w = 1.0;
  std::cout << "low num of different keys, all exclusive locks " << std::endl;
  lock_requests = GenerateRequests(n, k, w);
  Benchmark(lock_requests);
  lock_requests.clear();
  std::cout << "======" << std::endl;

  k = 5;
  w = 0.5;
  std::cout << "low num of different keys, mixed locks 50/50" << std::endl;
  lock_requests = GenerateRequests(n, k, w);
  Benchmark(lock_requests);
  lock_requests.clear();
  std::cout << "======" << std::endl;

  // all different configs
  // for (k = 5; k < 50; k+=5) {
  //   for (w = 0.0; w <= 1.0; w += 0.2) {
  //     std::cout << "100000 requests, "<< k << " keys, " << w*100 << "\% write locks" << std::endl;
  //     lock_requests = GenerateRequests(n, k, w);
  //     Benchmark(lock_requests);
  //     lock_requests.clear();
  //     std::cout << "======" << std::endl;
  //   }
  // }
}

std::vector<std::pair<Key, LockRequest>> Tester::GenerateRequests(int n, int k, double w) {
  std::vector<std::pair<Key, LockRequest>> lock_requests;

  for (int i = 0; i < n; i++) {
    Txn txn = Txn(1);
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
  std::string types[] = { "Global Lock", "Key Lock", "Latch-Free"  };
  for (int i = 0; i < 2; i++) {
    switch(i) {
      case 0:
        lm = new GlobalLockManager(100);
        break;
      case 1:
        lm = new KeyLockManager(100);
        break;
      //case 2:
        //lm = new LatchFreeLockManager();
        //break;
      default:
        break;
    }
    double throughput[2];

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
    throughput[0] = lock_requests.size() / (end-start);


    start = GetTime();
    // release phase
    for (auto it = lock_requests.begin(); it != lock_requests.end(); ++it) {
      Key k = it->first;
      LockRequest r = it->second;
      lm->Release(r.txn_, k);
    }

    end = GetTime();

    throughput[1] = lock_requests.size() / (end-start);

    std::cout << "Lock Manager: " << types[i] << std::endl;
    std::cout << "  Avg # Acquires / sec: " << int(throughput[0]) << std::endl
              << "  Avg # Releases / sec: " << int(throughput[1]) << std::endl;
  }
}
