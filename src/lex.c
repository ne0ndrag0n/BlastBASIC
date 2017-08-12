#include "lex.h"
#include "utility.h"
#include <stdlib.h>
#include <stdio.h>

Lexer* gsOpenLexer( char* buffer ) {
  Lexer* lexer = malloc( sizeof( Lexer ) );

  lexer->line = 0;
  lexer->start = 0;
  lexer->current = 0;

  lexer->buffer = buffer;

  return lexer;
}

Token* gsCreateToken( TokenType type ) {
  Token* token = calloc( 1, sizeof( Token ) );
  token->type = type;

  return token;
}

void gsCloseLexer( Lexer* lexer ) {
  free( lexer->buffer );
  free( lexer );
}

Lexer* gsGetLexerFromFile( const char* filename ) {
  FILE* file = fopen( filename, "r" );
  if( !file ) {
    fprintf( stderr, "%s: Unable to open %s", __FUNCTION__, filename );
    return NULL;
  }

  unsigned long fileLen = 0;
  fseek( file, 0, SEEK_END );
  fileLen = ftell( file );
  fseek( file, 0, SEEK_SET );

  char* buffer = malloc( sizeof( char ) * ( fileLen + 1 ) );
  if( !buffer ) {
    fprintf( stderr, "%s: Unable to allocate buffer", __FUNCTION__ );
  }

  fgets( buffer, fileLen, file );
  fclose( file );

  return gsOpenLexer( buffer );
}

List* gsLex( Lexer* lexer ) {
  List* result = NULL;
  List* current = NULL;
  List* prev = NULL;

  lexer->line = lexer->start = lexer->current = 0;

  char* character = lexer->buffer;
  while( character ) {

    current = utilCreateList();

    switch( *character ) {
      case '(': {
        current->data = gsCreateToken( LEFT_PAREN );
        break;
      }
      case ')': {
        current->data = gsCreateToken( RIGHT_PAREN );
        break;
      }
      case '{': {
        current->data = gsCreateToken( LEFT_BRACE );
        break;
      }
      case '}': {
        current->data = gsCreateToken( RIGHT_BRACE );
        break;
      }
      case ',': {
        current->data = gsCreateToken( COMMA );
        break;
      }
      case '.': {
        current->data = gsCreateToken( DOT );
        break;
      }
      case '-': {
        current->data = gsCreateToken( MINUS );
        break;
      }
      case '+': {
        current->data = gsCreateToken( PLUS );
        break;
      }
      case ';': {
        current->data = gsCreateToken( SEMICOLON );
        break;
      }
      case '*': {
        current->data = gsCreateToken( STAR );
        break;
      }
      default:
        break;
    }

    if( prev ) {
      prev->next = current;
    } else {
      // First token
      result = current;
    }

    prev = current;

    character++;
  }

  return result;
}
