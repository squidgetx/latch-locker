#ifndef _TXN_H_
#define _TXN_H_

class LockManager;

#include <vector>
#include "util/mutex.h"
#include "util/common.h"
#include "hashtable/TNode.h"
#include "lock_request.h"

#define REQUESTS_PER_TRANSACTION 20
class Txn {
  public:
    Txn(int txn, std::vector<std::pair<Key, LockMode>> q, int spin) : txn_id(txn), keys(q), spintime(spin) {}
    Txn(int txn, std::vector<std::pair<Key, LockMode>> q) : txn_id(txn), keys(q), spintime(0) {}
    Txn(int txn) : txn_id(txn) {}

    void Execute(LockManager *lm, int id, TNode<LockRequest>* lock_requests);

    Pthread_mutex txn_mutex;
    int txn_id;
    int spintime;
    std::vector<std::pair<Key, LockMode>> keys;

};

#endif
