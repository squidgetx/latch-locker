#ifndef HASHTABLE_TNODE_H
#define HASHTABLE_TNODE_H
#include <stdint.h>

template <typename T>
struct TNode // Node is a private implementation detail
{
    T data;
    TNode<T> *next;
    TNode<T> *prev;
    TNode<T>(const T d) : data(d), next(NULL), prev(NULL) {}

};

template <typename T>
struct MemoryChunk {
	MemoryChunk(T* loc, int64_t size): loc(loc), size(size) {}
	T* loc;
	int64_t size;
};

#endif
