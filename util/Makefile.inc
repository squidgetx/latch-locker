DIR=util
UTIL_SOURCES = $(wildcard $(DIR)/*.cc)
UTIL_OBJECTS = $(patsubst %.cc,%.o,$(UTIL_SOURCES))

SOURCES += $(UTIL_SOURCES)
