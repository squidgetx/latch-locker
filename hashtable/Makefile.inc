DIR=hashtable
HASHTABLE_SOURCES = $(wildcard $(DIR)/*.cc)
HASHTABLE_OBJECTS = $(patsubst %.cc, %.o, $(HASHTABLE_SOURCES))

SOURCES += $(HASHTABLE_SOURCES)
BINS += hashtest

hashtest: $(HASHTABLE_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^
