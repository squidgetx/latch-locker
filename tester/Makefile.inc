DIR=tester

TESTER_SOURCES = $(patsubst %, $(DIR)/%, tester.cc test.cc correct_test.cc)
TESTER_OBJECTS = $(patsubst %.cc, %.o, $(TESTER_SOURCES))

$(DIR)/tester.o : $(DIR)/tester.h

SOURCES += $(TESTER_SOURCES)
