#include "correct_test.h"
#include "util/testing.h"
#include <pthread.h>
#include <queue>


bool lockGranted(const TNode<LockRequest> *lr) {
  return lr->data.state_ == ACTIVE;
}

CorrectTester::CorrectTester() {

}

struct lha {
  Txn *txn_;
  //key, exclusive
  std::queue<std::pair<int, bool>> *queue_;
  LockManager *lm_;
  bool releaseLocks;
  lha(Txn *t, std::queue<std::pair<int, bool>> *q, LockManager *l,
      bool releaseLocks) :
      txn_(t), queue_(q), lm_(l), releaseLocks(releaseLocks) {};
};

// acquires and releases
void *t_lock_acquirer(void *args) {
  struct lha *lha = (struct lha*)args;
  int *grants = new int;
  *grants = 0;
  int rejects = 0;
  int id = lha->txn_->txn_id;

  std::vector<int> keys;
  while (true) {
    if (lha->queue_->size() == 0) {
      break;
    }
    std::pair<int, bool> p = lha->queue_->front();
    lha->queue_->pop();

    if (p.second) {
      if (lockGranted(lha->lm_->TryWriteLock(lha->txn_, p.first))) {
        (*grants)++;
      } else {
        rejects++;
      }
    } else {
      if (lockGranted(lha->lm_->TryReadLock(lha->txn_, p.first))) {
        (*grants)++;
      } else {
        rejects++;
      }
    }
    keys.push_back(p.first);
  }

  // Stall for a little while to simulate transaction workload
  // (time between lock acquire + release)
  for (int i = 0; i < 10000000; i++) {
    single_work();
  }

  if (lha->releaseLocks) {
    for (int i = 0; i < keys.size(); i++) {
      lha->lm_->Release(lha->txn_, keys[i]);
    }
  }

  pthread_exit((void*)grants);
}

void CorrectTester::MultithreadedLocking(LockManager *lm) {
  BEGIN;
  int NUM_THREADS = 4;
  int REPEATS = 4;
  pthread_t pthreads[NUM_THREADS];

  int grants;

  // 1
  // Have each thread request a shared lock and confirm each obtain
  // it successfully
  for (int i = 0; i < NUM_THREADS; i++) {
    std::vector<std::pair<Key, LockRequest>> q;
    Txn * txn = new Txn(i, q);
    std::queue<std::pair<int, bool>> * request_q = new std::queue<std::pair<int, bool>>();
    std::pair<int, bool> * p = new std::pair<int, bool>(1, false);
    request_q->push(*p);
    struct lha * args = new struct lha(txn, request_q, lm, true);
    pthread_create(&pthreads[i], NULL, t_lock_acquirer, (void*)args);
  }

  grants = 0;
  for (int i = 0; i < NUM_THREADS; i++) {
    int *g;
    pthread_join(pthreads[i], (void**)&g);
    grants += *g;
  }

  std::cout << "Threaded: all shared locks granted " << std::endl;
  EXPECT_EQ(grants, NUM_THREADS);

  // 2
  // all write locks, same key. Make sure only 1 thread got a lock

  std::vector<Txn*> txn_q;
  for (int i = 0; i < NUM_THREADS; i++) {
    std::vector<std::pair<Key, LockRequest>> q;
    Txn * txn = new Txn(i, q);
    txn_q.push_back(txn);
    std::queue<std::pair<int, bool>> * request_q = new std::queue<std::pair<int, bool>>();
    std::pair<int, bool> * p = new std::pair<int, bool>(1, true);
    request_q->push(*p);
    struct lha * args = new struct lha(txn, request_q, lm, false);
    pthread_create(&pthreads[i], NULL, t_lock_acquirer, (void*)args);
  }

  grants = 0;
  for (int i = 0; i < NUM_THREADS; i++) {
    int *g;
    pthread_join(pthreads[i], (void**)&g);
    grants += *g;
  }

  for (auto it = txn_q.begin(); it != txn_q.end(); it++) {
    lm->Release(*it, 1);
  }

  std::cout << "Threaded: only one write lock granted " << grants << std::endl;
  EXPECT_EQ(grants, 1);

  END;
}

void CorrectTester::SimpleLocking(LockManager * lm) {
  BEGIN;
  std::vector<std::pair<Key, LockRequest>> q;
  Txn * t1 = new Txn(1,q);
  Txn * t2 = new Txn(2,q);
  Txn * t3 = new Txn(3,q);
  Txn * t4 = new Txn(4,q);
  std::cout << "Shared locks don't need to wait" << std::endl;
  EXPECT_TRUE(lockGranted(lm->TryReadLock(t1, 1)));
  EXPECT_TRUE(lockGranted(lm->TryReadLock(t2, 1)));

  std::cout << "Locks release without breaking" << std::endl;
  lm->Release(t1, 1);
  lm->Release(t2, 1);

  std::cout << "First Exclusive lock doesn't need to wait" << std::endl;
  EXPECT_TRUE(lockGranted(lm->TryWriteLock(t1, 1)));

  std::cout << "Second exclusive lock needs to wait" << std::endl;
  EXPECT_FALSE(lockGranted(lm->TryWriteLock(t2, 1)));

  std::cout << "Shared lock doesn't need to wait after releasing exclusive" << std::endl;
  lm->Release(t1, 1);
  lm->Release(t2, 1);
  EXPECT_TRUE(lockGranted(lm->TryReadLock(t1, 1)));

  std::cout << "Additional exclusive must wait" << std::endl;
  EXPECT_FALSE(lockGranted(lm->TryWriteLock(t3, 1)));

  std::cout << "Additional shared must wait" << std::endl;
  EXPECT_FALSE(lockGranted(lm->TryReadLock(t4, 1)));

  std:: cout << "Additional shared acquires if exclusive is released" << std::endl;
  lm->Release(t1, 1);
  lm->Release(t3, 1);
  EXPECT_TRUE(lockGranted(lm->TryReadLock(t3, 1)));

  lm->Release(t1,1);
  lm->Release(t3,1);
  lm->Release(t4,1);
  END;
}

/**
 * Test various edge cases associated with releasing locks.
 */
void CorrectTester::ReleaseCases(LockManager *lm) {
  BEGIN;
  std::vector<std::pair<Key, LockRequest>> q;
  Txn *t1 = new Txn(1, q);
  Txn *t2 = new Txn(2, q);
  Txn *t3 = new Txn(3, q);
  Txn *t4 = new Txn(4, q);

  // Notation for all test cases is a sequence of locks, with the lock to be
  // released in brackets.
  //
  // Locks that are granted are in astericks.
  //
  // Each test uses a different key to ensure there is not interference.

  // *[SHARED]* *SHARED* EXCLUSIVE
  // *SHARED* EXCLUSIVE
  EXPECT_TRUE(lockGranted(lm->TryReadLock(t1, 1)));
  EXPECT_TRUE(lockGranted(lm->TryReadLock(t2, 1)));
  EXPECT_FALSE(lockGranted(lm->TryWriteLock(t3, 1)));

  lm->Release(t1, 1);
  EXPECT_TRUE(
      lm->CheckState(t1, 1) == NOT_FOUND || lm->CheckState(t1, 1) == OBSOLETE);
  EXPECT_EQ(lm->CheckState(t2, 1), ACTIVE);
  EXPECT_EQ(lm->CheckState(t3, 1), WAIT);

  // *SHARED* *[SHARED]* EXCLUSIVE
  // *SHARED* EXCLUSIVE SHARED
  EXPECT_TRUE(lockGranted(lm->TryReadLock(t1, 2)));
  EXPECT_TRUE(lockGranted(lm->TryReadLock(t2, 2)));
  EXPECT_FALSE(lockGranted(lm->TryWriteLock(t3, 2)));

  lm->Release(t2, 2);
  EXPECT_EQ(lm->CheckState(t1, 2), ACTIVE);
  EXPECT_TRUE(
      lm->CheckState(t2, 2) == NOT_FOUND || lm->CheckState(t2, 2) == OBSOLETE);
  EXPECT_EQ(lm->CheckState(t3, 2), WAIT);

  // *[EXCLUSIVE]* SHARED SHARED EXCLUSIVE
  // *SHARED* *SHARED* EXCLUSIVE
  EXPECT_TRUE(lockGranted(lm->TryWriteLock(t1, 3)));
  EXPECT_FALSE(lockGranted(lm->TryReadLock(t2, 3)));
  EXPECT_FALSE(lockGranted(lm->TryReadLock(t3, 3)));
  EXPECT_FALSE(lockGranted(lm->TryWriteLock(t4, 3)));

  lm->Release(t1, 3);
  EXPECT_TRUE(
      lm->CheckState(t1, 3) == NOT_FOUND || lm->CheckState(t1, 3) == OBSOLETE);
  EXPECT_EQ(lm->CheckState(t2, 3), ACTIVE);
  EXPECT_EQ(lm->CheckState(t3, 3), ACTIVE);
  EXPECT_EQ(lm->CheckState(t4, 3), WAIT);

  END;
}

void CorrectTester::Run() {
  // Test the global lcok manager
  // three types of mgr_s
  LockManager * lm;
  for (int i = 0; i < 3; i++) {
    switch(i) {
      case 0:
        lm = new GlobalLockManager(100);
        std::cout << "** Global Manager **" << std::endl;
        break;
      case 1:
        lm = new KeyLockManager(100);
        std::cout << "** Key Lock Manager **" << std::endl;
        break;
      case 2:
        lm = new LatchFreeLockManager(100);
        std::cout<< "** Latch Free Lock Manager **" << std::endl;
        break;
      default:
        break;
    }
    SimpleLocking(lm);
    MultithreadedLocking(lm);
    ReleaseCases(lm);
  }
}
