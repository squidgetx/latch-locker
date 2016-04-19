#ifndef _TXN_H_
#define _TXN_H_

#include "util/mutex.h"

class Txn {
  public:
    Pthread_mutex txn_mutex;

};

#endif
