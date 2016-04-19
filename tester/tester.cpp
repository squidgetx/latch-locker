#include <iostream>
#include <cstdlib>

#include "tester.h"

Tester::Tester() : mgrs({ GlobalLockManager(), LatchedLockManager(), LatchFreeLockManager() }) {
}

// run different tests
void Tester::Run() {
	std::vector<LockRequest> lock_requests;

	int n = 10000; // number of lock requests
	int k = 10; // number of distinct keys
	double w = 0.5; // percentage of write locks

	lock_requests = GenerateRequests(n, k, w);
	Benchmark(lock_requests);
	lock_requests.clear();

}

std::vector<LockRequest> Tester::GenerateRequests(int n, int k, double w) {
	std::vector<LockRequest> lock_requests;

	for (int i = 0; i < n; i++) {
		Txn txn = Txn();
		Key key = std::randint(1, k) % k;
		LockMode mode = (((double) rand() / (RAND_MAX)) <= w) ? EXCLUSIVE : SHARED;		
		LockRequest r = LockRequest(mode, &txn, key);
		lock_requests.push_back(r);
	}

	return lock_requests;
}


void Tester::Benchmark(std::vector<LockRequest> lock_requests) {

	// three types of mgr_s
	for (int i = 0; i < 3; i++) {
			double durations[2];

			// Record start time.
		  double start = GetTime();

			// acquire phase
			for(std::vector<T>::iterator it = lock_requests.begin(); it != lock_requests.end(); ++it) {
				LockRequest r = *it;
				if (r.mode_ == SHARED) {
					mgr[i]->ReadLock(r.txn, r.key);
				} else if (r.mode == EXCLUSIVE) {
					mgr[i]->WriteLock(r.txn, r.key);
				}
			}

			// Record end time
			double end = GetTime();
			durations[0] = lock_requests.size() / (end-start);


			start = GetTime();
			// release phase
			for(std::vector<T>::iterator it = lock_requests.begin(); it != lock_requests.end(); ++it) {
				LockRequest r = *it;
				mgr[i]->Release(r.txn, r.key);
			}
			end = GetTime();

			durations[1] = lock_requests.size() / (end-start);

			std::cout << "Lock Manager " << i << ". Avg Acquire Duration: " << duration[0] 
																				<< ". Avg Release Duration: " << duration[1] << std::endl; 
	}
}
