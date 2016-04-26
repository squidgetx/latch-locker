#ifndef _TXN_H_
#define _TXN_H_

class LockManager;

#include <queue>
#include "util/mutex.h"
#include "util/common.h"
#include "lock_request.h"

class Txn {
  public:
    Txn(int txn, std::queue<std::pair<Key, LockRequest>> q) : txn_id(txn), lr_queue(q) {}

    void Execute(LockManager *lm);

    Pthread_mutex txn_mutex;
    int txn_id;
    std::queue<std::pair<Key, LockRequest>> lr_queue;


};

#endif