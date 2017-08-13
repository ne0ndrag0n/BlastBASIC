#include "lex.h"
#include "utility.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

Lexer* gsOpenLexer( char* buffer ) {
  Lexer* lexer = malloc( sizeof( Lexer ) );

  lexer->line = 0;
  lexer->column = 0;

  lexer->buffer = buffer;
  lexer->currentCharacter = buffer;

  return lexer;
}

List* gsCreateToken( TokenType type ) {
  Token* token = calloc( 1, sizeof( Token ) );
  token->type = type;

  List* list = utilCreateList();
  list->data = token;

  return list;
}

void gsCloseLexer( Lexer* self ) {
  free( self->buffer );
  free( self );
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
    return NULL;
  }

  buffer[ fread( buffer, fileLen, 1, file ) ] = 0;
  fclose( file );

  return gsOpenLexer( buffer );
}

List* gsPeekSet( Lexer* self, char check, TokenType ifTrue, TokenType ifFalse ) {
  char peek = *( self->currentCharacter + 1 );

  if( peek == check ) {
    // Safe to consume the next character
    self->currentCharacter++;
    self->column++;

    return gsCreateToken( ifTrue );
  } else {
    return gsCreateToken( ifFalse );
  }
}

/**
 * Lexer buffer MUST be null-terminated before running the lexer. gsGetLexerFromFile usually handles this for you, but gsOpenLexer does not.
 */
List* gsLex( Lexer* self ) {
  List* result = NULL;
  List* current = NULL;
  List* prev = NULL;

  self->line = self->column = 1;
  self->currentCharacter = self->buffer;

  while( *self->currentCharacter ) {

    switch( *self->currentCharacter ) {
      case '(': {
        current = gsCreateToken( LEFT_PAREN );
        break;
      }
      case ')': {
        current = gsCreateToken( RIGHT_PAREN );
        break;
      }
      case '{': {
        current = gsCreateToken( LEFT_BRACE );
        break;
      }
      case '}': {
        current = gsCreateToken( RIGHT_BRACE );
        break;
      }
      case ',': {
        current = gsCreateToken( COMMA );
        break;
      }
      case '.': {
        current = gsCreateToken( DOT );
        break;
      }
      case '-': {
        current = gsCreateToken( MINUS );
        break;
      }
      case '+': {
        current = gsCreateToken( PLUS );
        break;
      }
      case ';': {
        current = gsCreateToken( SEMICOLON );
        break;
      }
      case '*': {
        current = gsCreateToken( STAR );
        break;
      }
      case '!': {
        current = gsPeekSet( self, '=', BANG_EQUAL, BANG );
        break;
      }
      case '=': {
        current = gsPeekSet( self, '=', EQUAL_EQUAL, EQUAL );
        break;
      }
      case '<': {
        current = gsPeekSet( self, '=', LESS_EQUAL, EQUAL );
        break;
      }
      case '>': {
        current = gsPeekSet( self, '=', GREATER_EQUAL, EQUAL );
        break;
      }
      case '/': {
        if( *( self->currentCharacter + 1 ) == '/' ) {
          // Single-line comment
          // Advance currentCharacter/column to nearest 0 or \n
          while( *self->currentCharacter && *self->currentCharacter != '\n' ) {
            self->currentCharacter++;
            self->column++;
          }

          continue;
        } else {
          current = gsCreateToken( SLASH );
        }
        break;
      }
      case ' ':
      case '\r':
      case '\t': {
        self->currentCharacter++;
        self->column++;
        continue;
      }
      case '\n': {
        self->currentCharacter++;
        self->column = 1;
        self->line++;
        continue;
      }
      case '"': {
        size_t stringSize = 0;

        self->currentCharacter++;
        if( *self->currentCharacter == '\n' ) {
          self->column = 1;
          self->line++;
        } else {
          self->column++;
        }

        const char* begin = self->currentCharacter;
        while( *self->currentCharacter && *self->currentCharacter != '"' ) {
          stringSize++;

          self->currentCharacter++;
          if( *self->currentCharacter == '\n' ) {
            self->column = 1;
            self->line++;
          } else {
            self->column++;
          }
        }

        // If at the end of the file or an empty string, we'll get here without stringSize being incremented

        if( *self->currentCharacter == 0 ) {
          printf( "Unterminated string!" );
          continue;
        }

        current = gsCreateToken( STRING );
        char* string = ( ( Token* ) current->data )->literal.asString = calloc( stringSize + 1, sizeof( char ) );
        if( stringSize ) {
          strncpy( string, begin, stringSize );
        }

        break;
      }
      default: {
        if( *self->currentCharacter >= '0' && *self->currentCharacter <= '9' ) {
          char* begin = self->currentCharacter;

          size_t stringSize = 0;
          bool isHex = false;
          bool isBin = false;

          // check for hex
          if( *self->currentCharacter == '0' && *( self->currentCharacter + 1 ) == 'x' ) {
            isHex = true;
            stringSize = 2;
            self->currentCharacter += 2;
            self->column += 2;
          }

          // TODO
        } else {
          printf( "Unexpected character at (%d, %d): %c", self->line, self->column, *self->currentCharacter );
          self->currentCharacter++;
          self->column++;
          continue;
        }
      }
    }

    if( prev ) {
      prev->next = current;
    } else {
      // First token
      result = current;
    }

    prev = current;

    self->currentCharacter++;
    self->column++;
  }

  return result;
}
