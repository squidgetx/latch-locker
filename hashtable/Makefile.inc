DIR=hashtable
HASHTABLE_SOURCES = $(patsubst %, $(DIR)/%, hashtable.cc LockRequestLinkedList.cc)
HASHTABLE_OBJECTS = $(patsubst %.cc, %.o, $(HASHTABLE_SOURCES))

SOURCES += $(HASHTABLE_SOURCES)
BINS += hashtest

hashtest: $(HASHTABLE_OBJECTS) $(DIR)/test.o
	$(CXX) $(CXXFLAGS) -o $@ $^
