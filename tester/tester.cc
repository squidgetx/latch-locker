#include "tester.h"

#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <iterator>
#include <random>
#include <algorithm>

#include "lock_managers/global_lock_manager.h"
#include "lock_managers/lock_manager.h"
#include "util/common.h"
#include "lock_request.h"

static inline double GetTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec/1e6;
}

struct txn_handler {
  std::queue<Txn*> *txn_queue_;
  LockManager *lm_;
  txn_handler(std::queue<Txn*> *t,
      LockManager *l) : txn_queue_(t), lm_(l) {}
};

Tester::Tester() {
}

// run different tests
void Tester::Run() {
  Txn *txn;
  std::vector<Txn*> * transactions = new std::vector<Txn*>();

  int m = TRANSACTIONS_PER_TEST; // transactions per benchmarktest
  int n = REQUESTS_PER_TRANSACTION; // number of lock requests per transaction
  int k; // number of distinct keys
  double w; // percentage of write locks

  k = 99;
  w = 0.0;
  std::cout << "high num of different keys, all shared locks " << std::endl;
  for (int i = 0; i < m; i++) {
    transactions->push_back(GenerateTransaction(n, k, w));
  }
  Benchmark(transactions);
  transactions->clear();
  std::cout << "======" << std::endl;

  /*
  k = 100;
  w = 1.0;
  std::cout << "high num of different keys, all exclusive locks " << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push_back(GenerateTransaction(n, k, w));
  }
  Benchmark(transactions);
  transactions.clear();
  std::cout << "======" << std::endl;

  k = 100;
  w = 0.5;
  std::cout << "high num of different keys, mixed locks 50/50 " << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push_back(GenerateTransaction(n, k, w));
  }
  Benchmark(transactions);
  transactions.clear();
  std::cout << "======" << std::endl;

  k = 5;
  w = 0.0;
  std::cout << "low num of different keys, all shared locks " << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push_back(GenerateTransaction(n, k, w));
  }
  Benchmark(transactions);
  transactions.clear();
  std::cout << "======" << std::endl;

  k = 5;
  w = 1.0;
  std::cout << "low num of different keys, all exclusive locks " << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push_back(GenerateTransaction(n, k, w));
  }
  Benchmark(transactions);
  transactions.clear();
  std::cout << "======" << std::endl;

  k = 5;
  w = 0.5;
  std::cout << "low num of different keys, mixed locks 50/50" << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push_back(GenerateTransaction(n, k, w));
  }
  Benchmark(transactions);
  transactions.clear();
  std::cout << "======" << std::endl;*/
}

bool comparator(std::pair<Key, LockMode> a, std::pair<Key, LockMode> b) {
  return (a.first > b.first);
}

Txn *Tester::GenerateTransaction(int n, int k, double w) {
  // generates a single txn that acts on N keys
  // choosing the keys from a pool of [0,K]
  // w is proportion of write keys
//  std::cout << "Generating Txn: " << txn_counter << " : ";
  std::vector<std::pair<Key, LockMode>> lock_requests;

  for (int i = 0; i < n; i++) {
    // make sure you are getting a unique key
    Key key = 1 + (rand() % (int) k);
    for(int j = 0; j < i; j++) {
      if (lock_requests[j].first == key) {
        key = 1 + (rand() % (int) k);
        j = -1; 
        continue;
      }
    }
    LockMode mode = (((double) rand() / (RAND_MAX)) <= w) ? EXCLUSIVE : SHARED;
    lock_requests.push_back(std::make_pair(key, mode));
  }
  std::sort(lock_requests.begin(), lock_requests.end(), comparator);

  Txn *t = new Txn(txn_counter, lock_requests);
  txn_counter++;

  return t;
}

void *threaded_transactions_executor(void *args) {
  // Record start time.
  double start = GetTime();

  struct txn_handler *tha = (struct txn_handler *)args;

  int num_txns = tha->txn_queue_->size();
 // std::cout << num_txns << std::endl;

  while (tha->txn_queue_->size()) {
    Txn *t = tha->txn_queue_->front();
    tha->txn_queue_->pop();    
    t->Execute(tha->lm_);
  }

  // Record end time
  double end = GetTime();

  double *throughput = new double;
  *throughput = num_txns / (end-start);

  return (void*) throughput;
}

void Tester::Benchmark(std::vector<Txn*> * transactions) {
  LockManager *lm;
  // three types of mgr_s
  std::string types[] = { "Global Lock", "Key Lock", "Latch-Free"};
  for (int i = 0; i < 3; i++) {
    switch(i) {
      case 0:
        lm = new GlobalLockManager(100);
        break;
      case 1:
        lm = new KeyLockManager(100);
        break;
      case 2:
        lm = new LatchFreeLockManager(100);
        break;
      default:
        break;
    }

    int throughput = 0;

    for (int j = 0; j < NUM_THREADS; j++) {

      std::queue<Txn*> * txns = new std::queue<Txn*>();
      for (int k = 0; k < TRANSACTIONS_PER_TEST/NUM_THREADS; k++) {
        txns->push((*transactions)[TRANSACTIONS_PER_TEST/NUM_THREADS*j + k]);
      }

      struct txn_handler * tha = new struct txn_handler(txns, lm);
      pthread_create(&pthreads[j], NULL, threaded_transactions_executor, (void*) tha);
    }

    for (int j = 0; j < NUM_THREADS; j++) {
      double *tput;
      pthread_join(pthreads[j], (void**)&tput);
      throughput += *tput;
      std::cout << *tput << std::endl;
      delete tput;
    }

    throughput /= NUM_THREADS;

    std::cout << "Lock Manager: " << types[i] << std::endl;
    std::cout << "    Txn throughput / sec: " << throughput << std::endl;
  }
}
