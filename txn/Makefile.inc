DIR=txn

TXN_SOURCES = $(DIR)/txn.cc
TXN_OBJECTS = $(patsubst %.cc, %.o, $(TXN_SOURCES))

SOURCES += $(TXN_SOURCES)