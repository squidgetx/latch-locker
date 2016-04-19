CXX=g++
CXXFLAGS=--std=c++11 -I$(CURDIR)

DIRS=util lock_managers hashtable
SOURCES :=
BINS :=
OBJECTS = $(patsubst %.cc, %.o, $(SOURCES))

all:

include $(patsubst %, %/Makefile.inc, $(DIRS))

.PHONY: all
all: $(OBJECTS) $(BINS)

.PHONY: clean
clean:
	rm `find . '-name' '*.o'`
	rm -f $(BINS)
