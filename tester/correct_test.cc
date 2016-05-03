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

   std::vector<std::pair<int,TNode<LockRequest>*>> keys;
   while (true) {
     if (lha->queue_->size() == 0) {
       break;
     }
     std::pair<int, bool> p = lha->queue_->front();
     lha->queue_->pop();

     TNode<LockRequest>* lr;
     if (p.second) {
       lr = new TNode<LockRequest>(LockRequest(EXCLUSIVE,lha->txn_));
       if (lockGranted(lha->lm_->TryWriteLock(lr,p.first))) {
         (*grants)++;
       } else {
         rejects++;
       }
     } else {
       lr = new TNode<LockRequest>(LockRequest(SHARED,lha->txn_));
       if (lockGranted(lha->lm_->TryReadLock(lr,p.first))) {
         (*grants)++;
       } else {
         rejects++;
       }
     }
     keys.push_back(std::pair<int,TNode<LockRequest>*>(p.first,lr));
   }

   // Stall for a little while to simulate transaction workload
   // (time between lock acquire + release)
   for (int i = 0; i < 10000000; i++) {
     single_work();
   }

   if (lha->releaseLocks) {
     for (int i = 0; i < keys.size(); i++) {
       lha->lm_->Release(keys[i].second, keys[i].first);
     }
   }

   pthread_exit((void*)grants);
 }

/*
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
     Txn * txn = new Txn(i);
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
     Txn * txn = new Txn(i);
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
 */

 void CorrectTester::SimpleLocking(LockManager * lm) {
   BEGIN;
   Txn * t1 = new Txn(1);
   Txn * t2 = new Txn(2);
   Txn * t3 = new Txn(3);
   Txn * t4 = new Txn(4);
   std::cout << "Shared locks don't need to wait" << std::endl;
   TNode<LockRequest> l1(LockRequest(SHARED,t1));
   TNode<LockRequest> l2(LockRequest(SHARED,t2));

   EXPECT_TRUE(lockGranted(lm->TryReadLock(&l1, 1)));
   EXPECT_TRUE(lockGranted(lm->TryReadLock(&l2, 1)));

   std::cout << "Locks release without breaking" << std::endl;
   lm->Release(&l1, 1);
   lm->Release(&l2, 1);

   std::cout << "First Exclusive lock doesn't need to wait" << std::endl;
   TNode<LockRequest> l3(LockRequest(EXCLUSIVE,t1));
   EXPECT_TRUE(lockGranted(lm->TryWriteLock(&l3, 1)));

   std::cout << "Second exclusive lock needs to wait" << std::endl;
   TNode<LockRequest> l4(LockRequest(EXCLUSIVE,t2));
   EXPECT_FALSE(lockGranted(lm->TryWriteLock(&l4, 1)));

   std::cout << "Shared lock doesn't need to wait after releasing exclusive" << std::endl;
   lm->Release(&l3, 1);
   lm->Release(&l4, 1);

   TNode<LockRequest> l5(LockRequest(SHARED,t1));

   EXPECT_TRUE(lockGranted(lm->TryReadLock(&l5, 1)));

   std::cout << "Additional exclusive must wait" << std::endl;

   TNode<LockRequest> l6(LockRequest(EXCLUSIVE,t3));
   EXPECT_FALSE(lockGranted(lm->TryWriteLock(&l6, 1)));

   std::cout << "Additional shared must wait" << std::endl;
   TNode<LockRequest> l7(LockRequest(SHARED,t4));
   EXPECT_FALSE(lockGranted(lm->TryReadLock(&l7, 1)));

   std:: cout << "Additional shared acquires if exclusive is released" << std::endl;
   lm->Release(&l5, 1);
   lm->Release(&l6, 1);
   TNode<LockRequest> l8(LockRequest(SHARED,t3));
   EXPECT_TRUE(lockGranted(lm->TryReadLock(&l8, 1)));

   lm->Release(&l7,1);
   lm->Release(&l8,1);
   END;
 }

 /**
  * Test various edge cases associated with releasing locks.
  */
 void CorrectTester::ReleaseCases(LockManager *lm) {
   BEGIN;
   Txn *t1 = new Txn(1);
   Txn *t2 = new Txn(2);
   Txn *t3 = new Txn(3);
   Txn *t4 = new Txn(4);

   // Notation for all test cases is a sequence of locks, with the lock to be
   // released in brackets.
   //
   // Locks that are granted are in astericks.
   //
   // Each test uses a different key to ensure there is not interference.
   TNode<LockRequest> * l1 = new TNode<LockRequest>(LockRequest(SHARED,t1));
   TNode<LockRequest> * l2 = new TNode<LockRequest>(LockRequest(SHARED,t2));
   TNode<LockRequest> * l3 = new TNode<LockRequest>(LockRequest(EXCLUSIVE,t3));

   // *[SHARED]* *SHARED* EXCLUSIVE
   // *SHARED* EXCLUSIVE
   EXPECT_TRUE(lockGranted(lm->TryReadLock(l1, 1)));
   EXPECT_TRUE(lockGranted(lm->TryReadLock(l2, 1)));
   EXPECT_FALSE(lockGranted(lm->TryWriteLock(l3, 1)));

   lm->Release(l1, 1);

   EXPECT_TRUE(
       lm->CheckState(t1, 1) == NOT_FOUND || lm->CheckState(t1, 1) == OBSOLETE);
   EXPECT_EQ(lm->CheckState(t2, 1), ACTIVE);
   EXPECT_EQ(lm->CheckState(t3, 1), WAIT);
  
   // *SHARED* *[SHARED]* EXCLUSIVE
   // *SHARED* EXCLUSIVE SHARED
   l1 = new TNode<LockRequest>(LockRequest(SHARED, t1));
   l2 = new TNode<LockRequest>(LockRequest(SHARED, t2));
   l3 = new TNode<LockRequest>(LockRequest(EXCLUSIVE, t3));
   EXPECT_TRUE(lockGranted(lm->TryReadLock(l1, 2)));
   EXPECT_TRUE(lockGranted(lm->TryReadLock(l2, 2)));
   EXPECT_FALSE(lockGranted(lm->TryWriteLock(l3, 2)));

   lm->Release(l2, 2);
   EXPECT_EQ(lm->CheckState(t1, 2), ACTIVE);
   EXPECT_TRUE(
       lm->CheckState(t2, 2) == NOT_FOUND || lm->CheckState(t2, 2) == OBSOLETE);
   EXPECT_EQ(lm->CheckState(t3, 2), WAIT);

   l1 = new TNode<LockRequest>(LockRequest(EXCLUSIVE, t1));
   l2 = new TNode<LockRequest>(LockRequest(SHARED, t2));
   l3 = new TNode<LockRequest>(LockRequest(SHARED, t3));
   TNode<LockRequest> * l4 = new TNode<LockRequest>(LockRequest(EXCLUSIVE, t4));

   // *[EXCLUSIVE]* SHARED SHARED EXCLUSIVE
   // *SHARED* *SHARED* EXCLUSIVE
   EXPECT_TRUE(lockGranted(lm->TryWriteLock(l1, 3)));
   EXPECT_FALSE(lockGranted(lm->TryReadLock(l2, 3)));
   EXPECT_FALSE(lockGranted(lm->TryReadLock(l3, 3)));
   EXPECT_FALSE(lockGranted(lm->TryWriteLock(l4, 3)));

   lm->Release(l1, 3);
   EXPECT_TRUE(
       lm->CheckState(t1, 3) == NOT_FOUND || lm->CheckState(t1, 3) == OBSOLETE);
   EXPECT_EQ(lm->CheckState(t2, 3), ACTIVE);
   EXPECT_EQ(lm->CheckState(t3, 3), ACTIVE);
   EXPECT_EQ(lm->CheckState(t4, 3), WAIT);

   // *[EXCLUSIVE]* EXCLUSIVE
   // *EXCLUSIVE*

   l1 = new TNode<LockRequest>(LockRequest(EXCLUSIVE, t1));
   l2 = new TNode<LockRequest>(LockRequest(EXCLUSIVE, t2));

   EXPECT_TRUE(lockGranted(lm->TryWriteLock(l1, 4)));
   EXPECT_FALSE(lockGranted(lm->TryWriteLock(l2, 4)));

   lm->Release(l1, 4);
   EXPECT_TRUE(
       lm->CheckState(t1, 4) == NOT_FOUND || lm->CheckState(t1, 4) == OBSOLETE);
   EXPECT_EQ(lm->CheckState(t2, 4), ACTIVE);

   // *[SHARED]* EXCLUSIVE
   // *EXCLUSIVE*

   l1 = new TNode<LockRequest>(LockRequest(SHARED, t1));
   l2 = new TNode<LockRequest>(LockRequest(EXCLUSIVE, t2));

   EXPECT_TRUE(lockGranted(lm->TryReadLock(l1, 5)));
   EXPECT_FALSE(lockGranted(lm->TryWriteLock(l2, 5)));

   lm->Release(l1, 5);
   EXPECT_TRUE(
       lm->CheckState(t1, 5) == NOT_FOUND || lm->CheckState(t1, 5) == OBSOLETE);
   EXPECT_EQ(lm->CheckState(t2, 5), ACTIVE);

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
       case 2:
         lm = new LatchFreeLockManager(100);
         std::cout<< "** Latch Free Lock Manager **" << std::endl;
         break;
       default:
         break;
     }
     SimpleLocking(lm);
     //MultithreadedLocking(lm);
     ReleaseCases(lm);
   }
 }
