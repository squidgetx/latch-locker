#ifndef _TXN_H_
#define _TXN_H_

class LockManager;

#include <vector>
#include "util/mutex.h"
#include "util/common.h"
#include "hashtable/TNode.h"
#include "lock_request.h"

#define TRANSACTIONS_PER_TEST 100000
#define REQUESTS_PER_TRANSACTION 20

class Txn {
  public:
    Txn(int txn, std::vector<std::pair<Key, LockMode>> q) : txn_id(txn), keys(q) {}
    Txn(int txn) : txn_id(txn) {}

    void Execute(LockManager *lm, int id, TNode<LockRequest>* lock_requests);

    Pthread_mutex txn_mutex;
    int txn_id;
    std::vector<std::pair<Key, LockMode>> keys;


};

#endif
