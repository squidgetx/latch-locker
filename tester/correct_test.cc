 #include "correct_test.h"
 #include "util/testing.h"
 #include <pthread.h>
 #include <queue>


 bool lockGranted(const TNode<LockRequest> *lr) {
   return lr->data.state_ == ACTIVE;
 }

 CorrectTester::CorrectTester() {

 }

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
     ReleaseCases(lm);
   }
 }
