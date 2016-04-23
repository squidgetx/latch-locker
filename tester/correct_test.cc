#include "correct_test.h"
#include "util/testing.h"
#include <pthread.h>
#include <queue>

int NUM_THREADS = 10;

CorrectTester::CorrectTester() {

}

struct lha {
  Txn *txn_;
  //key, exclusive
  std::queue<std::pair<int, bool>> *queue_;
  pthread_mutex_t *queue_mutex_;
  LockManager *lm_;
  lha(Txn *t, std::queue<std::pair<int, bool>> *q, pthread_mutex_t *qm, LockManager *l) : 
      txn_(t), queue_(q), queue_mutex_(qm), lm_(l) {};
};

// acquires and releases
void *t_lock_acquirer(void *args) {
  struct lha *lha = (struct lha*)args;
  int *grants = new int;
  *grants = 0;
  int rejects = 0;

  std::vector<int> keys;
  while (lha->queue_->size()) {
    pthread_mutex_lock(lha->queue_mutex_);
    if (lha->queue_->size() == 0) {
      pthread_mutex_unlock(lha->queue_mutex_);
      break;
    }
    std::pair<int, bool> p = lha->queue_->front();
    lha->queue_->pop();
    pthread_mutex_unlock(lha->queue_mutex_);

    if (p.second) {
      if (lha->lm_->WriteLock(lha->txn_, p.first)) {
        (*grants)++;
      } else {
        rejects++;
      }
    } else {
      if (lha->lm_->ReadLock(lha->txn_, p.first)) {
        (*grants)++;
      } else {
        rejects++;
      }
    }
    keys.push_back(p.first);
  }
  for (int i = 0; i < keys.size(); i++) {
    lha->lm_->Release(lha->txn_, keys[i]);
  }

  pthread_exit((void*)grants);
}

void CorrectTester::MultithreadedLocking(LockManager *lm) {
  BEGIN;
  int NUM_THREADS = 4;
  pthread_t pthreads[NUM_THREADS];

  int NUM_LOCKS = 500;

  //key, exclusive bool
  //queue and vector of the same thing
  std::queue<std::pair<int, bool>> q;
  pthread_mutex_t m;
  pthread_mutex_init(&m, NULL);
  int grants;

  // 1
  // all read locks, same key
  for (int i = 0; i < NUM_LOCKS; i++) {
    std::pair<int, bool> p(1, false);
    q.push(p);
  }
  
  for (int i = 0; i < NUM_THREADS; i++) {
    Txn t(i);
    struct lha args(&t, &q, &m, lm);
    pthread_create(&pthreads[i], NULL, t_lock_acquirer, (void*)&args);
  }

  grants = 0;
  for (int i = 0; i < NUM_THREADS; i++) {
    int *g;
    pthread_join(pthreads[i], (void**)&g);
    grants += *g;
  }

  std::cout << "Threaded: all shared locks granted" << std::endl;
  EXPECT_TRUE((grants == NUM_LOCKS));

  // 2
  // all write locks, same key

  for (int i = 0; i < NUM_LOCKS; i++) {
    std::pair<int, bool> p(2, true);
    q.push(p);
  }
  
  for (int i = 0; i < NUM_THREADS; i++) {
    Txn t(i);
    struct lha args(&t, &q, &m, lm);
    pthread_create(&pthreads[i], NULL, t_lock_acquirer, (void*)&args);
  }

  grants = 0;
  for (int i = 0; i < NUM_THREADS; i++) {
    int *g;
    pthread_join(pthreads[i], (void**)&g);
    grants += *g;
  }

  std::cout << "Threaded: one write lock granted" << std::endl;
  EXPECT_TRUE((grants == 1));

  // 3
  // releases; all locks should have been released
  // and all lock requests should thus be granted
  for (int i = 0; i < NUM_LOCKS; i++) {
    std::pair<int, bool> p(2, false);
    q.push(p);
  }
  
  for (int i = 0; i < NUM_THREADS; i++) {
    Txn t(i);
    struct lha args(&t, &q, &m, lm);
    pthread_create(&pthreads[i], NULL, t_lock_acquirer, (void*)&args);
  }

  grants = 0;
  for (int i = 0; i < NUM_THREADS; i++) {
    int *g;
    pthread_join(pthreads[i], (void**)&g);
    grants += *g;
  }

  std::cout << "Threaded: releases correctly" << std::endl;
  EXPECT_TRUE((grants == NUM_LOCKS));

  END;
}

void CorrectTester::SimpleLocking(LockManager * lm) {
  BEGIN;
  Txn * t1 = new Txn(1);
  Txn * t2 = new Txn(2);
  Txn * t3 = new Txn(3);
  Txn * t4 = new Txn(4);
  std::cout << "Shared locks don't need to wait" << std::endl;
  bool result = lm->ReadLock(t1, 1);
  EXPECT_TRUE(result);
  result = lm->ReadLock(t2, 1);
  EXPECT_TRUE(result);

  std::cout << "Locks release without breaking" << std::endl;
  lm->Release(t1, 1);
  lm->Release(t2, 1);

  std::cout << "First Exclusive lock doesn't need to wait" << std::endl;
  EXPECT_TRUE(lm->WriteLock(t1, 1));

  std::cout << "Second exclusive lock needs to wait" << std::endl;
  EXPECT_FALSE(lm->WriteLock(t2, 1 ));

  std::cout << "Shared lock doesn't need to wait after releasing exclusive" << std::endl;
  lm->Release(t2, 1);
  lm->Release(t1, 1);
  EXPECT_TRUE(lm->ReadLock(t1, 1));

  std::cout << "Additional exclusive must wait" << std::endl;
  EXPECT_FALSE(lm->WriteLock(t3, 1 ));

  std::cout << "Additional shared must wait" << std::endl;
  EXPECT_FALSE(lm->ReadLock(t4, 1 ));

  std:: cout << "Additional shared acquires if exclusive is released" << std::endl;
  lm->Release(t3, 1);
  EXPECT_TRUE(lm->ReadLock(t3, 1));

  lm->Release(t1,1);
  lm->Release(t3,1);
  lm->Release(t4,1);
  END;
}

void CorrectTester::Run() {
  // Test the global lcok manager
  // three types of mgr_s
  LockManager * lm;
  for (int i = 0; i < 2; i++) {
    switch(i) {
      case 0:
        lm = new GlobalLockManager(100);
        std::cout << "** Global Manager **" << std::endl;
        break;
      case 1:
        lm = new KeyLockManager(100);
        std::cout << "** Key Lock Manager **" << std::endl;
        break;
      //case 2:
        //lm = new LatchFreeLockManager();
        //break;
      default:
        break;
    }
    SimpleLocking(lm);
    MultithreadedLocking(lm); 
  }
}
