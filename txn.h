#ifndef _TXN_H_
#define _TXN_H_

#include "util/mutex.h"

class Txn {
  public:
    Txn(int txn) : txn_id(txn) {};
    Pthread_mutex txn_mutex;
    int txn_id;

};

#endif
