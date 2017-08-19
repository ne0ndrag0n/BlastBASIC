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

List* gsProcessNumeric( Lexer* self ) {
  List* current = NULL;

  char* begin = self->currentCharacter;
  size_t stringSize = 0;

  // check for hex
  if( *self->currentCharacter == '0' && *( self->currentCharacter + 1 ) == 'x' ) {
    // Hexadecimal case
    stringSize = 2;
    self->currentCharacter += 2;
    self->column += 2;

    while(
      ( *self->currentCharacter >= '0' && *self->currentCharacter <= '9' ) ||
      ( *self->currentCharacter >= 'A' && *self->currentCharacter <= 'F' ) ||
      ( *self->currentCharacter >= 'a' && *self->currentCharacter <= 'f' )
    ) {
      self->currentCharacter++;
      self->column++;
      stringSize++;
    }

    current = gsCreateToken( INTEGER );

    char value[ stringSize + 1 ];
    strncpy( value, begin, stringSize );
    value[ stringSize ] = 0;
    ( ( Token* ) current->data )->literal.asInteger = strtoul( value, NULL, 16 );

  } else {
    // Non-hexadecimal case (decimal, binary maybe in the future)
    // A numeric token has ZERO or ONE period.
    bool isReal = false;

    while( ( *self->currentCharacter >= '0' && *self->currentCharacter <= '9' ) || ( *self->currentCharacter == '.' && !isReal ) ) {
      if( *self->currentCharacter == '.' ) {
        isReal = true;
      }

      self->currentCharacter++;
      self->column++;
      stringSize++;
    }

    if( stringSize == 0 ) {
      // You, the developer, didn't call this function properly if no string was created.
      // gsProcessNumeric should only ever be called if you have something that can become a number token
      printf( "Invalid number token (should never get here)!" );
      return NULL;
    }

    // Create literal
    char value[ stringSize + 1 ];
    strncpy( value, begin, stringSize );
    value[ stringSize ] = 0;

    // Cannot end on a period, also cannot have a "number" which is just a period
    if( value[ stringSize - 1 ] == '.' || ( stringSize == 1 && value[ 0 ] == '.' ) ) {
      printf( "Malformed decimal!" );
      return NULL;
    }

    current = gsCreateToken( REAL );

    // Interpret the value differently as real
    if( isReal ) {
      ( ( Token* ) current->data )->literal.asDouble = strtod( value, NULL );
    } else {
      ( ( Token* ) current->data )->literal.asInteger = strtoul( value, NULL, 10 );
    }

  }

  return current;
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
        // Peek ahead for number - it may be a numeric token with just a leading "."
        if( *( self->currentCharacter + 1 ) >= '0' && *( self->currentCharacter + 1 ) <= '9' ) {
          current = gsProcessNumeric( self );
          if( !current ) {
            // Couldn't create a numeric token
            continue;
          }
        } else {
          current = gsCreateToken( DOT );
        }
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
          current = gsProcessNumeric( self );
          if( !current ) {
            // Couldn't create a numeric token
            continue;
          }
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
