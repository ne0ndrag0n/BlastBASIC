#include "lex.h"
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

List_Token* gsCreateToken( TokenType type ) {
  List_Token* token = calloc( 1, sizeof( List_Token ) );

  token->data.type = type;

  return token;
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

List_Token* gsPeekSet( Lexer* self, char check, TokenType ifTrue, TokenType ifFalse ) {
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

List_Token* gsProcessNumeric( Lexer* self ) {
  List_Token* current = NULL;

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
    current->data.literal.asInteger = strtoul( value, NULL, 16 );

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
      current->data.literal.asDouble = strtod( value, NULL );
    } else {
      current->data.literal.asInteger = strtoul( value, NULL, 10 );
    }

  }

  return current;
}

bool gsIsAlpha( char c ) {
  return ( c >= 'A' && c <= 'Z' ) ||
         ( c >= 'a' && c <= 'z' ) ||
         c == '_';
}

bool gsIsAlphanumeric( char c ) {
  return gsIsAlpha( c ) ||
    ( c >= '0' && c <= '9' );
}

List_Token* gsGetReservedWordOrIdentifier( char* identifier ) {
  // TODO !!
  return NULL;
}

/**
 * Lexer buffer MUST be null-terminated before running the lexer. gsGetLexerFromFile usually handles this for you, but gsOpenLexer does not.
 */
List_Token* gsLex( Lexer* self ) {
  List_Token* result = NULL;
  List_Token* current = NULL;
  List_Token* prev = NULL;

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
      case '%': {
        current = gsCreateToken( MODULO );
        break;
      }
      case '^': {
        current = gsCreateToken( BITWISE_XOR );
        break;
      }
      case '~': {
        current = gsCreateToken( ONES_COMPLIMENT );
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
      case '&': {
        current = gsPeekSet( self, '&', AND, BITWISE_AND );
        break;
      }
      case '|': {
        current = gsPeekSet( self, '|', OR, BITWISE_OR );
        break;
      }
      case '<': {
        switch( *( self->currentCharacter + 1 ) ) {
          case '=': {
            current = gsCreateToken( LESS_EQUAL );
            break;
          }
          case '<': {
            current = gsCreateToken( LEFT_SHIFT );
            break;
          }
          default: {
            current = gsCreateToken( LESS );
            break;
          }
        }

        break;
      }
      case '>': {
        switch( *( self->currentCharacter + 1 ) ) {
          case '=': {
            current = gsCreateToken( GREATER_EQUAL );
            break;
          }
          case '>': {
            current = gsCreateToken( RIGHT_SHIFT );
            break;
          }
          default: {
            current = gsCreateToken( GREATER );
            break;
          }
        }

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
        char* string = current->data.literal.asString = calloc( stringSize + 1, sizeof( char ) );
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
        } else if ( gsIsAlpha( *self->currentCharacter ) ) {
          // Possible identifier or reserved word
          const char* begin = self->currentCharacter;
          size_t stringSize = 0;

          while( gsIsAlphanumeric( *self->currentCharacter ) ) {
            self->currentCharacter++;
            self->column++;
            stringSize++;
          }

          char value[ stringSize + 1 ];
          strncpy( value, begin, stringSize );
          value[ stringSize ] = 0;

          current = gsGetReservedWordOrIdentifier( value );
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
