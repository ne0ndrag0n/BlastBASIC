#include "parse.h"
#include "lex.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


List_String* gsGetScopeResolution( Parser* self ) {
  if( self->state.current->data.type != IDENTIFIER ) {
    return NULL;
  }

  List_String* first = calloc( 1, sizeof( List_String ) );
  first->data = self->state.current->data.literal.asString;

  gsParserIncrement( self );

  List_String* prev = first;
  // While next two types are DOT and IDENTIFIER in a sequence
  while( self->state.current->data.type == DOT && self->state.current->next && self->state.current->next->data.type == IDENTIFIER ) {
    // Eat the DOT
    gsParserIncrement( self );

    prev->next = calloc( 1, sizeof( List_String ) );
    prev->next->data = self->state.current->data.literal.asString;
    prev = prev->next;

    // Eat that IDENTIFIER we just used
    gsParserIncrement( self );
  }

  return first;
}

ASTNode* gsGetPackageStatement( Parser* self ) {
  if( self->state.current->data.type != PACKAGE ) {
    return NULL;
  }

  List_String* scopeResolution = gsGetScopeResolution( self );
  if( !scopeResolution ) {
    if( !self->error ) {
      const char* preamble = "Unexpected ";
      const char* suffix = gsGetDebugOutput( self->state.current );

      self->error = calloc( strlen( preamble ) + strlen( suffix ) + 1, sizeof( char ) );
      strcat( self->error, preamble );
      strcat( self->error, suffix );
    }

    return NULL;
  }
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
