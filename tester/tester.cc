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

  int m = TRANSACTIONS_PER_TEST; // transactions per benchmarktest
  int n = REQUESTS_PER_TRANSACTION; // number of lock requests per transaction
  double w = 1;


  NUM_THREADS = 64;

  std::cout << "Test contention. Fixed 4 cores" << std::endl;
  srand (time(NULL));

  /*
  // vary contention
  for (double hs_size = 0.05; hs_size > 0.00001; hs_size/=2) {
    std::vector<Key> hot_set;
    std::vector<Key> cold_set;
    for (int i = 0; i < KEYS; i++) {
      if ((rand() % 100) < hs_size * 100) {
        hot_set.push_back(i);
      } else {
        cold_set.push_back(i);
      }
    }

    std::cout << "Contention: Hot set size " << hs_size << ". All write locks " << std::endl;
    for (int i = 0; i < m; i++) {
      transactions->push_back(GenerateTransaction(n, w, hot_set, cold_set));
    }
    Benchmark(transactions);
    transactions->clear();
    std::cout << "======" << std::endl;
  }

  */

  std::cout << "Test thread influence. Low Contention. Fixed 0.05 hot set size" << std::endl;
  for (int num_threads = 1; num_threads <= 256; num_threads *= 2) {
    NUM_THREADS = num_threads;
    std::vector<Key> hot_set;
    std::vector<Key> cold_set;

    double hs_size = 0.05;
    for (int i = 0; i < KEYS; i++) {
      if ((rand() % 100) < hs_size * 100) {
        hot_set.push_back(i);
      } else {
        cold_set.push_back(i);
      }
    }

    std::cout << "Threads: Num threads " << NUM_THREADS << ". All write locks " << std::endl;
    for (int i = 0; i < m; i++) {
      transactions->push_back(GenerateTransaction(n, w, hot_set, cold_set));
    }
    Benchmark(transactions);
    transactions->clear();
    std::cout << "======" << std::endl;
  }

  std::cout << "Test thread influence. High Contention. Fixed 0.001 hot set size" << std::endl;
  for (int num_threads = 1; num_threads <= 512; num_threads *= 2) {
    NUM_THREADS = num_threads;
    std::vector<Key> hot_set;
    std::vector<Key> cold_set;

    double hs_size = 0.001;
    for (int i = 0; i < KEYS; i++) {
      if ((rand() % 100) < hs_size * 100) {
        hot_set.push_back(i);
      } else {
        cold_set.push_back(i);
      }
    }

    std::cout << "Threads: Num threads " << NUM_THREADS << ". All write locks " << std::endl;
    for (int i = 0; i < m; i++) {
      transactions->push_back(GenerateTransaction(n, w, hot_set, cold_set));
    }
    Benchmark(transactions);
    transactions->clear();
    std::cout << "======" << std::endl;
  }
}

bool comparator(std::pair<Key, LockMode> a, std::pair<Key, LockMode> b) {
  return (a.first > b.first);
}

Txn *Tester::GenerateTransaction(int n, double w, std::vector<Key> hot_set, std::vector<Key> cold_set) {
  // generates a single txn that acts on N keys
  // choosing the keys from a pool of [0,K]
  // w is proportion of write keys
  std::vector<std::pair<Key, LockMode>> lock_requests;
  std::set<Key> keys;


  for (int i = 0; i < n; i++) {
	Key key;
	do {
    key = (i < NUM_HOT_REQUESTS) ? hot_set[(rand() % (int) hot_set.size())] :    // random key from hot set
                          cold_set[(rand() % (int)cold_set.size())];    // random key from cold set
	}
	while (keys.count(key));
	keys.insert(key);


    // ensure unique keys
	/*
	if (key
	keys.insert(key)
    for(int j = 0; j < i; j++) {
      if (lock_requests[j].first == key) {
        key = (i < NUM_HOT_REQUESTS) ? hot_set[(rand() % (int)hot_set.size())] :    // random key from hot set
                          cold_set[(rand() % (int)cold_set.size())];    // random key from cold set
        j = -1; 
      }
    }
	*/
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

  double *throughput = new double;
  *throughput = num_txns / (end-start);


  free(membuffer);
  return (void*) throughput;
}

void Tester::Benchmark(std::vector<Txn*> * transactions) {
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
     // std::cout << *tput << std::endl;
      delete tput;
    }

   // throughput /= NUM_THREADS;

    std::cout << "Lock Manager: " << types[i] << std::endl;
    std::cout << "    Txn throughput / sec: " << throughput << std::endl;
  }
}
