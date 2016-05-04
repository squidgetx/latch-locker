#include "tester.h"

#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <iterator>
#include <random>
#include <algorithm>
#include <cassert>
#include <set>
#include <stdlib.h>

#include "lock_managers/global_lock_manager.h"
#include "lock_managers/lock_manager.h"
#include "util/common.h"
#include "lock_request.h"

#define DEFAULT_SPIN_TIME 10000
#define NUM_HOT_REQUESTS 20
#define KEYS 100000
#define TRANSACTIONS_PER_TEST 500000

#define THREAD_SIZE_LENGTH 19
static int threadSizes[] = {1,2,4,8,16,24,32,40,48,56,64,80,96,112,128,160,196,228,256};


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

// run different tests
void Tester::Run() {
  Txn *txn;
  std::vector<Txn*> * transactions = new std::vector<Txn*>();

  double w = 1;

  std::cout << "Test contention. Fixed 64 cores" << std::endl;
  srand (time(NULL));
  std::vector<Key> hot_set;
  std::vector<Key> cold_set;

  // vary contention
  for (int hs_size = KEYS; hs_size > 0.05*KEYS; hs_size -= 5000) {

    for (int i = 0; i < KEYS; i++) {
      if (i < hs_size) {
        hot_set.push_back(i);
      } else {
        cold_set.push_back(i);
      }
    }

    std::cout << "Contention: Hot set size " << hs_size << ". All write locks " << std::endl;
    for (int i = 0; i < TRANSACTIONS_PER_TEST; i++) {
      transactions->push_back(GenerateTransaction(w, hot_set, cold_set));
    }
    Benchmark(transactions, 64);
    transactions->clear();
    hot_set.clear();
    cold_set.clear();
    std::cout << "======" << std::endl;
  }

  std::cout << "Test thread influence. Low Contention. Fixed 0.05 hot set size" << std::endl;
  for (int i = 0; i < THREAD_SIZE_LENGTH; i++ ) {
    int nthreads = threadSizes[i];

    double hs_size = KEYS;
    for (int i = 0; i < KEYS; i++) {
      if (i < hs_size) {
        hot_set.push_back(i);
      } else {
        cold_set.push_back(i);
      }
    }

    std::cout << "Threads: Num threads " << nthreads << ". All write locks " << std::endl;
    for (int i = 0; i < TRANSACTIONS_PER_TEST; i++) {
      transactions->push_back(GenerateTransaction(w, hot_set, cold_set));
    }
    Benchmark(transactions, nthreads);
    transactions->clear();
    hot_set.clear();
    cold_set.clear();
    std::cout << "======" << std::endl;
  }

  std::cout << "Test thread influence. High Contention. Fixed 0.001 hot set size" << std::endl;
  for (int i = 0; i < THREAD_SIZE_LENGTH; i++ ) {
    int nthreads = threadSizes[i];

    double hs_size = 0.05*KEYS;
    for (int i = 0; i < KEYS; i++) {
      if (i < hs_size) {
        hot_set.push_back(i);
      } else {
        cold_set.push_back(i);
      }
    }

    std::cout << "Threads: Num threads " << nthreads << ". All write locks " << std::endl;
    for (int i = 0; i < TRANSACTIONS_PER_TEST; i++) {
      transactions->push_back(GenerateTransaction(w, hot_set, cold_set));
    }
    Benchmark(transactions, nthreads);
    transactions->clear();
    cold_set.clear();
    hot_set.clear();
    std::cout << "======" << std::endl;
  }
}

bool comparator(std::pair<Key, LockMode> a, std::pair<Key, LockMode> b) {
  return (a.first > b.first);
}

Txn *Tester::GenerateTransaction(double w, std::vector<Key> &hot_set,
    std::vector<Key> &cold_set) {
  // choosing the keys from a pool of [0,K]
  // w is proportion of write keys
  std::vector<std::pair<Key, LockMode>> lock_requests;
  std::set<Key> keys;


  for (int i = 0; i < REQUESTS_PER_TRANSACTION; i++) {
    Key key;
    do {
      key = (i < NUM_HOT_REQUESTS) ? hot_set[(rand() % (int) hot_set.size())] :    // random key from hot set
                            cold_set[(rand() % (int) cold_set.size())];    // random key from cold set
    }
    while (keys.count(key));
    keys.insert(key);

    LockMode mode = (((double) rand() / (RAND_MAX)) <= w) ? EXCLUSIVE : SHARED;
    lock_requests.push_back(std::make_pair(key, mode));
  }
  std::sort(lock_requests.begin(), lock_requests.end(), comparator);

  Txn *t = new Txn(txn_counter, lock_requests, DEFAULT_SPIN_TIME);
  txn_counter++;

  return t;
}

struct thread_info {
  double throughput;
  void* membuffer;
  thread_info(double tput, void* mb) : throughput(tput), membuffer(mb) {};
};

void *threaded_transactions_executor(void *args) {
  // Record start time.

  struct txn_handler *tha = (struct txn_handler *)args;

  int num_txns = tha->txn_queue_->size();
 // std::cout << num_txns << std::endl;
  int executed = 0;
  int id = 0;
  void* membuffer = valloc(sizeof(TNode<LockRequest>)*REQUESTS_PER_TRANSACTION*TRANSACTIONS_PER_TEST);
  TNode<LockRequest>* lock_requests = reinterpret_cast<TNode<LockRequest>*> (membuffer);
  
  double start = GetTime();

  while (tha->txn_queue_->size()) {
    Txn *t = tha->txn_queue_->front();
    tha->txn_queue_->pop();    
    t->Execute(tha->lm_, id, lock_requests);
    id++;
 /*   
    executed++;
    if (executed % 1000 == 0) {
      std::cout << executed << "\r";
    }*/
  }
  //std::cout << std::endl;

  // Record end time
  double end = GetTime();

  double throughput = num_txns / (end-start);


//  free(membuffer);
  struct thread_info * i = new struct thread_info(throughput, membuffer);
  return (void*) i;
}

void Tester::Benchmark(std::vector<Txn*> * transactions, int nthreads) {
  LockManager *lm;
  // three types of mgr_s
  std::string types[] = { "Global Lock", "Key Lock", "Latch-Free"};
  for (int i = 0; i < 3; i++) {
    switch(i) {
      case 0:
        lm = new GlobalLockManager(KEYS);
        break;
      case 1:
        lm = new KeyLockManager(KEYS);
        break;
      case 2:
        lm = new LatchFreeLockManager(KEYS);
        break;
      default:
        break;
    }

    long long int throughput = 0;

    for (int j = 0; j < nthreads; j++) {

      std::queue<Txn*> * txns = new std::queue<Txn*>();
      for (int k = 0; k < TRANSACTIONS_PER_TEST/nthreads; k++) {
        txns->push((*transactions)[TRANSACTIONS_PER_TEST/nthreads*j + k]);
      }

      struct txn_handler * tha = new struct txn_handler(txns, lm);
      pthread_create(&pthreads[j], NULL, threaded_transactions_executor, (void*) tha);
    }

    struct thread_info * infos[nthreads];

    for (int j = 0; j < nthreads; j++) {
      struct thread_info * t;
      pthread_join(pthreads[j], (void**)&t);
      infos[j] = t;
      throughput += t->throughput;
     // std::cout << *tput << std::endl;
    }
    for(int j = 0; j < nthreads; j++) {
      free(infos[j]->membuffer);
    }


    delete lm;


    std::cout << "Lock Manager: " << types[i] << std::endl;
    std::cout << "    Txn throughput / sec: " << throughput << std::endl;
  }
}
