#ifndef MIDAS_VM
#define MIDAS_VM

#define MIDAS_VM_RAM_SIZE 32768

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t Register;

typedef struct MidasVM {
  Register cmp1;
  Register cmp2;
  uint8_t* stackPointer;
  uint8_t* heapPointer;
  uint8_t* programCounter;
  uint8_t ram[ MIDAS_VM_RAM_SIZE ];
} MidasVM;

typedef struct TypeField {
  char* fieldName;
  uint16_t typeIndex;
} TypeField;

typedef struct TypeDescriptor {
  uint16_t blockSize;
  char* typeName;
  uint16_t numFields;
  TypeField* typeFields;
} TypeDescriptor;

typedef struct HeapHeader {
  bool free : 1;
  bool last : 1;
  bool array : 1;
  bool mark : 1;
  uint16_t typeIndex : 12;
} HeapHeader;

typedef struct StackHeader {
  bool pointer : 1;
  uint8_t slotSize : 7;
} StackHeader;

MidasVM* gsGetMidasVM();

#endif
