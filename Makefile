CXX=g++
CXXFLAGS=-pthread -g --std=c++11 -I$(CURDIR) -O3

DIRS=util lock_managers hashtable tester txn
SOURCES :=
BINS :=
OBJECTS = $(patsubst %.cc, %.o, $(SOURCES))

all:

include $(patsubst %, %/Makefile.inc, $(DIRS))

BINS += test_all
test_all: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

.PHONY: all
all: $(OBJECTS) $(BINS)

.PHONY: clean
clean:
	rm `find . '-name' '*.o'`
	rm -f $(BINS)

.PHONY: test
test: test_all
	./test_all
