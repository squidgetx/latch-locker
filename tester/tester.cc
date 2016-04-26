#include "tester.h"

#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <iterator>
#include <random>

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
  pthread_mutex_t *queue_mutex_;
  std::queue<Txn*> *txn_queue_;
  LockManager *lm_;
  txn_handler(pthread_mutex_t *m, std::queue<Txn*> *t,
      LockManager *l) : queue_mutex_(m), txn_queue_(t), lm_(l) {}
};

void clear( std::queue<Txn*> &q )
{
   std::queue<Txn*> empty;
   std::swap( q, empty );
}

Tester::Tester() {
}

// run different tests
void Tester::Run() {
  Txn *txn;
  std::queue<Txn*> transactions;

  int m = 1000; // transactions per benchmarktest
  int n = 10000; // number of lock requests per transaction
  int k; // number of distinct keys
  double w; // percentage of write locks

  k = 100;
  w = 0.0;
  std::cout << "high num of different keys, all shared locks " << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push(GenerateTransaction(n, k, w));
    Benchmark(transactions);
    clear(transactions);
  }
  std::cout << "======" << std::endl;

  k = 100;
  w = 1.0;
  std::cout << "high num of different keys, all exclusive locks " << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push(GenerateTransaction(n, k, w));
    Benchmark(transactions);
    clear(transactions);
  }
  std::cout << "======" << std::endl;

  k = 100;
  w = 0.5;
  std::cout << "high num of different keys, mixed locks 50/50 " << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push(GenerateTransaction(n, k, w));
    Benchmark(transactions);
    clear(transactions);
  }
  std::cout << "======" << std::endl;

  k = 5;
  w = 0.0;
  std::cout << "low num of different keys, all shared locks " << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push(GenerateTransaction(n, k, w));
    Benchmark(transactions);
    clear(transactions);
  }
  std::cout << "======" << std::endl;

  k = 5;
  w = 1.0;
  std::cout << "low num of different keys, all exclusive locks " << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push(GenerateTransaction(n, k, w));
    Benchmark(transactions);
    clear(transactions);
  }
  std::cout << "======" << std::endl;

  k = 5;
  w = 0.5;
  std::cout << "low num of different keys, mixed locks 50/50" << std::endl;
  for (int i = 0; i < m; i++) {
    transactions.push(GenerateTransaction(n, k, w));
    Benchmark(transactions);
    clear(transactions);
  }
  std::cout << "======" << std::endl;
}

Txn *Tester::GenerateTransaction(int n, int k, double w) {
  std::queue<std::pair<Key, LockRequest>> lock_requests;
  Txn *t = new Txn(txn_counter, lock_requests);

  for (int i = 0; i < n; i++) {
    Key key = 1 + (rand() % (int)k);
    LockMode mode = (((double) rand() / (RAND_MAX)) <= w) ? EXCLUSIVE : SHARED;
    LockRequest r = LockRequest(mode, t);
    lock_requests.push(std::make_pair(key, r));
  }

  return t;
}

void *threaded_transactions_executor(void *args) {
  // acquire phase
  struct txn_handler *tha = (struct txn_handler *)args;

  while (tha->txn_queue_->size()) {
    pthread_mutex_lock(tha->queue_mutex_);
    if (tha->txn_queue_->size() == 0) {
      pthread_mutex_unlock(tha->queue_mutex_);
      break;
    }
    Txn *t = tha->txn_queue_->front();
    tha->txn_queue_->pop();
    pthread_mutex_unlock(tha->queue_mutex_);
    
    t->Execute(tha->lm_);

  }
  pthread_exit(NULL);
}

void Tester::Benchmark(std::queue<Txn*> transactions) {
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

    struct txn_handler tha(&queue_mutex, &transactions, lm);

    double throughput;

    // Record start time.
    double start = GetTime();

    for (int j = 0; j < NUM_THREADS; j++) {
      pthread_create(&pthreads[j], NULL, threaded_transactions_executor, (void*)&tha);
    }

    for (int j = 0; j < NUM_THREADS; j++) {
      pthread_join(pthreads[j], NULL);
    }

    // Record end time
    double end = GetTime();
    throughput = transactions.size() / (end-start);

    std::cout << "Lock Manager: " << types[i] << std::endl;
    std::cout << "    Txn throughput / sec: " << int(throughput) << std::endl;
  }
}
