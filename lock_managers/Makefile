CC=g++
CCFLAGS=-O3 --std=c++11

CPP_SOURCES=$(wildcard *.cc)
CPP_HEADERS=$(wildcard *.h)

OBJS=$(patsubst %.cc,%.o,${CPP_SOURCES})

all: $(OBJS)
	@echo ${OBJS}
