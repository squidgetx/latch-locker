#include "correct_test.h"
#include "util/testing.h"

CorrectTester::CorrectTester() {

}

void CorrectTester::SimpleLocking(LockManager * lm) {
  BEGIN;
  Txn * t1 = new Txn();
  Txn * t2 = new Txn();
  Txn * t3 = new Txn();
  Txn * t4 = new Txn();
  std::cout << "Shared locks don't need to wait\n";
  bool result = lm->ReadLock(t1, 1);
  EXPECT_TRUE(result);
  result = lm->ReadLock(t2, 1);
  EXPECT_TRUE(result);

  std::cout << "Locks release without breaking\n";
  lm->Release(t1, 1);
  lm->Release(t2, 1);

  std::cout << "First Exclusive lock doesn't need to wait\n";
  EXPECT_TRUE(lm->WriteLock(t1, 1));

  std::cout << "Shared lock needs to wait\n";
  EXPECT_FALSE(lm->WriteLock(t2, 1 ));

  std::cout << "Shared lock doesn't need to wait after releasing exclusive\n";
  lm->Release(t1, 1);
  EXPECT_TRUE(lm->ReadLock(t1, 1));

  std::cout << "Additional exclusive must wait\n";
  EXPECT_FALSE(lm->WriteLock(t3, 1 ));

  std::cout << "Additional shared must wait\n";
  EXPECT_FALSE(lm->ReadLock(t4, 1 ));

  std:: cout << "Additional shared acquires if exclusive is released\n";
  lm->Release(t3, 1);
  EXPECT_TRUE(lm->ReadLock(t3, 1));

  END;
}

void CorrectTester::Run() {
  // Test the global lcok manager
  // three types of mgr_s
  LockManager * lm;
  for (int i = 0; i < 1; i++) {
    switch(i) {
      case 0:
        lm = new GlobalLockManager(100);
        break;
      //case 1:
        //lm = new LatchFreeLockManager();
        //break;
      //case 2:
        //lm = new LatchFreeLockManager();
        //break;
      default:
        break;
    }
  }
  SimpleLocking(lm);
  

}
