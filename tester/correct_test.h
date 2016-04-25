#ifndef _CORRECT_TESTER_H_
#define _CORRECT_TESTER_H_

#include <vector>
#include <utility>

#include "lock_managers/global_lock_manager.h"
#include "lock_managers/key_lock_manager.h"
#include "lock_managers/latch_free_lock_manager.h"
#include "util/common.h"
#include "txn.h"
#include "lock_request.h"
#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <iterator>
#include <random>

class CorrectTester {
  public:
    CorrectTester();
    void Run();
  private:
    void SimpleLocking(LockManager *lm);
    void ReleaseCases(LockManager *lm);
    void MultithreadedLocking(LockManager *lm);
};

#endif
