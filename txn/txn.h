#ifndef _TXN_H_
#define _TXN_H_

class LockManager;

#include <vector>
#include "util/mutex.h"
#include "util/common.h"
#include "lock_request.h"

class Txn {
  public:
    Txn(int txn, std::vector<std::pair<Key, LockMode>> q) : txn_id(txn), keys(q) {}
    Txn(int txn) : txn_id(txn) {}

    void Execute(LockManager *lm);

    Pthread_mutex txn_mutex;
    int txn_id;
    std::vector<std::pair<Key, LockMode>> keys;


};

#endif
