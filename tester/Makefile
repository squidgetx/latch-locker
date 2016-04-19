CC = g++
CFLAGS = -Wall -lstdc++

all: test

tester.o: tester.cpp tester.h ../lock_managers/lock_manager.h ../util/common.h ../txn.h
	$(CC) -Wall -c tester.cpp

test: test.o tester.o
	$(CC) $(CFLAGS) test.o tester.o -o test
