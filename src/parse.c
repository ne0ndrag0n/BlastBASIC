#include "parse.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char* CACHED_STRINGS[ GS_CACHED_STRING_MAX ] = { 0 };

char* gsGetCachedString( CachedString cachedString ) {
  return CACHED_STRINGS[ cachedString ];
}

CachedString gsCacheString( char* string ) {
  for( unsigned int i = 0; i != GS_CACHED_STRING_MAX; i++ ) {
    if( !strcmp( CACHED_STRINGS[ i ], string ) ) {
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
  size_t totalLen = strlen( self->state.current->data.literal.asString );
  List_Token* original = self->state.current;

  gsParserIncrement( self );

  
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
