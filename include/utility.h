#ifndef GOLDSCORPION_UTIL
#define GOLDSCORPION_UTIL

#include <stddef.h>

typedef struct Array {
  void* data;
  size_t size;
} Array;

typedef struct List {
  void* data;
  struct List* next;
} List;

List* utilCreateList();
void utilCloseList( List* first );

#endif
