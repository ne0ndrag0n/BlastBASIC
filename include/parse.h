#ifndef GOLDSCORPION_PARSE
#define GOLDSCORPION_PARSE

#define GS_CACHED_STRING_MAX 1024

#include <stddef.h>
#include <stdbool.h>
#include "lex.h"

typedef enum ASTNodeType {
  ASTPackageStatement,
  ASTScopeResolution
} ASTNodeType;

typedef struct List_String {
  char* data;
  struct List_String* next;
} List_String;

typedef struct ASTNode {
  ASTNodeType type;
  union {
    List_String* asPackageStatement;
    List_String* asScopeResolution;
  } data;
} ASTNode;

typedef struct StatePackage {
  List_Token* prev;
  List_Token* current;
} StatePackage;

typedef struct Parser {
  StatePackage state;
  char* error;
} Parser;

void gsParserIncrement( Parser* self );

Parser* gsGetParser( List_Token* starterToken );

#endif
