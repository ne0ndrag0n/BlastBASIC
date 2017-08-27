#ifndef GOLDSCORPION_PARSE
#define GOLDSCORPION_PARSE

#define GS_CACHED_STRING_MAX 1024

#include <stddef.h>
#include <stdbool.h>
#include "lex.h"

typedef unsigned int CachedString;

typedef CachedString PackageStatement;
typedef CachedString ScopeResolution;
typedef struct ImportStatement {
  CachedString* imports;
  size_t numImports;
  CachedString from;
} ImportStatement;

typedef enum ASTNodeType {
  ASTPackageStatement,
  ASTImportStatement,
  ASTScopeResolution
} ASTNodeType;

typedef struct ASTNode {
  ASTNodeType type;
  union {
    PackageStatement asPackageStatement;
    ImportStatement asImportStatement;
    ScopeResolution asScopeResolution;
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

char* gsGetCachedString( CachedString cachedString );
CachedString gsCacheString( char* string, bool freeIfFound );
void gsClearCachedStrings();

ASTNode* gsGetScopeResolution( Parser* self, bool test );
ASTNode* gsGetPackageDecl( Parser* self, bool test );
ASTNode* gsGetImportDecl( Parser* self, bool test );
ASTNode* gsGetStatement( Parser* self );
ASTNode* gsGetProgram( Parser* self );

void gsParserIncrement( Parser* self );

Parser* gsGetParser( List_Token* starterToken );

#endif
