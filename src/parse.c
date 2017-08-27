#include "parse.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char* CACHED_STRINGS[ GS_CACHED_STRING_MAX ] = { 0 };

char* gsGetCachedString( CachedString cachedString ) {
  return CACHED_STRINGS[ cachedString ];
}

CachedString gsCacheString( char* string, bool freeIfFound ) {
  for( unsigned int i = 0; i != GS_CACHED_STRING_MAX; i++ ) {
    if( !strcmp( CACHED_STRINGS[ i ], string ) ) {
      if( freeIfFound ) {
        free( string );
      }
      return i;
    }

    if( !CACHED_STRINGS[ i ] ) {
      CACHED_STRINGS[ i ] = string;
      return i;
    }
  }

  printf( "Out of cached strings.\n" );
  exit( 2 );
}

void gsClearCachedStrings() {
  for( unsigned int i = 0; i != GS_CACHED_STRING_MAX; i++ ) {
    free( CACHED_STRINGS[ i ] );
    CACHED_STRINGS[ i ] = 0;
  }
}

ASTNode* gsGetScopeResolution( Parser* self, bool test ) {
  if( self->state.current->data.type != IDENTIFIER ) {
    if( !test && !self->error ) {
      self->error = "Expected: IDENTIFIER token";
    }

    return NULL;
  }

  // At least one identifier - at this point, we'll have something to return
  int stringCount = 1;
  size_t totalLen = strlen( self->state.current->data.literal.asString );
  List_Token* scope = self->state.current;

  gsParserIncrement( self );

  // While the next two types are DOT and IDENTIFIER...
  while( self->state.current->data.type == DOT && self->state.current->next && self->state.current->next->data.type == IDENTIFIER ) {
    // eat the DOT
    gsParserIncrement( self );

    // add the identifier length to totalLen, plus one for the dot
    totalLen += ( 1 + strlen( self->state.current->data.literal.asString ) );

    // eat the IDENTIFIER
    gsParserIncrement( self );

    // this is another string
    stringCount++;
  }

  // Create a new string out of the objects starting at scope
  char* scopeString = calloc( totalLen + 1, sizeof( char ) );

  // Do the first one, special case
  memcpy( scopeString, scope->data.literal.asString, strlen( scope->data.literal.asString ) );
  stringCount--;

  for( ; stringCount != 0; stringCount-- ) {
    // Skip over the dot token
    scope = scope->next;

    strcat( scopeString, "." );
    strcat( scopeString, scope->data.literal.asString );

    // Skip to the next dot token
    scope = scope->next;
  }

  // Cache the string
  CachedString csId = gsCacheString( scopeString, true );

  ASTNode* result = calloc( 1, sizeof( ASTNode ) );
  result->type = ASTScopeResolution;
  result->data.asScopeResolution = csId;

  return result;
}

void gsParserIncrement( Parser* self ) {
  self->state.prev = self->state.current;
  self->state.current = self->state.current->next;
}

Parser* gsGetParser( List_Token* starterToken ) {
  Parser* parser = calloc( 1, sizeof( Parser ) );

  parser->state.current = starterToken;

  return parser;
}
