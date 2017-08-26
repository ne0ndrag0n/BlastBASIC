#include "lex.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
    printf( "Unable to open %s\n", filename );
    return NULL;
  }

  unsigned long fileLen = 0;
  fseek( file, 0, SEEK_END );
  fileLen = ftell( file );
  fseek( file, 0, SEEK_SET );

  char* buffer = calloc( fileLen + 1, sizeof( char ) );
  if( !buffer ) {
    printf( "Unable to allocate buffer\n" );
    return NULL;
  }

  fread( buffer, fileLen, 1, file );
  fclose( file );

  return gsOpenLexer( buffer );
}

List_Token* gsPeekSet( Lexer* self, char check, TokenType ifTrue, TokenType ifFalse ) {
  char peek = *( self->currentCharacter + 1 );

  if( peek == check ) {
    // Safe to consume the next character
    gsLexerIncrement( self );

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

    // Increment twice
    gsLexerIncrement( self );
    gsLexerIncrement( self );

    while(
      ( *self->currentCharacter >= '0' && *self->currentCharacter <= '9' ) ||
      ( *self->currentCharacter >= 'A' && *self->currentCharacter <= 'F' ) ||
      ( *self->currentCharacter >= 'a' && *self->currentCharacter <= 'f' )
    ) {
      gsLexerIncrement( self );
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

      gsLexerIncrement( self );
      stringSize++;
    }

    if( stringSize == 0 ) {
      // You, the developer, didn't call this function properly if no string was created.
      // gsProcessNumeric should only ever be called if you have something that can become a number token
      printf( "Invalid number token (should never get here)!\n" );
      return NULL;
    }

    // Create literal
    char value[ stringSize + 1 ];
    strncpy( value, begin, stringSize );
    value[ stringSize ] = 0;

    // Cannot end on a period, also cannot have a "number" which is just a period
    if( value[ stringSize - 1 ] == '.' || ( stringSize == 1 && value[ 0 ] == '.' ) ) {
      printf( "Malformed decimal!\n" );
      return NULL;
    }

    // Interpret the value differently as real
    if( isReal ) {
      current = gsCreateToken( REAL );
      current->data.literal.asDouble = strtod( value, NULL );
    } else {
      current = gsCreateToken( INTEGER );
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

bool gsIsNumeric( char c ) {
  return ( c >= '0' && c <= '9' );
}

bool gsIsAlphanumeric( char c ) {
  return gsIsAlpha( c ) || gsIsNumeric( c );
}

void gsLexerIncrement( Lexer* self ) {

  // Do not increment beyond allocated buffer
  if( *self->currentCharacter == 0 ) {
    return;
  }

  if( *self->currentCharacter == '\n' ) {
    // Advancing a line
    self->column = 1;
    self->line++;
  } else {
    // Just advance the column
    self->column++;
  }

  self->currentCharacter++;
}

/**
 * strLen doesn't include the null terminator
 */
List_Token* gsGetReservedWordOrIdentifier( char* identifier, size_t strLen ) {
  List_Token* result = NULL;

  // Return special token for any of the reserved keywords
  if( !strcmp( identifier, "class" ) ) {
    result = gsCreateToken( CLASS );
  } else if( !strcmp( identifier, "else" ) ) {
    result = gsCreateToken( ELSE );
  } else if( !strcmp( identifier, "false" ) ) {
    result = gsCreateToken( BOOL_FALSE );
  } else if( !strcmp( identifier, "true" ) ) {
    result = gsCreateToken( BOOL_TRUE );
  } else if( !strcmp( identifier, "for" ) ) {
    result = gsCreateToken( FOR );
  } else if( !strcmp( identifier, "if" ) ) {
    result = gsCreateToken( IF );
  } else if( !strcmp( identifier, "null" ) ) {
    result = gsCreateToken( NULL_TOKEN );
  } else if( !strcmp( identifier, "return" ) ) {
    result = gsCreateToken( RETURN );
  } else if( !strcmp( identifier, "super" ) ) {
    result = gsCreateToken( SUPER );
  } else if( !strcmp( identifier, "this" ) ) {
    result = gsCreateToken( THIS );
  } else if( !strcmp( identifier, "while" ) ) {
    result = gsCreateToken( WHILE );
  } else if( !strcmp( identifier, "static" ) ) {
    result = gsCreateToken( STATIC );
  } else if( !strcmp( identifier, "bool" ) ) {
    result = gsCreateToken( BOOL );
  } else if( !strcmp( identifier, "var" ) ) {
    result = gsCreateToken( VAR );
  } else if( !strcmp( identifier, "addr" ) ) {
    result = gsCreateToken( ADDR );
  } else if( !strcmp( identifier, "package" ) ) {
    result = gsCreateToken( PACKAGE );
  } else if( !strcmp( identifier, "import" ) ) {
    result = gsCreateToken( IMPORT );
  } else if( !strcmp( identifier, "from" ) ) {
    result = gsCreateToken( FROM );
  } else {
    // Check for primitive integer types
    int uint = !strncmp( identifier, "uint", 4 );
    int sint = !strncmp( identifier, "int", 3 );
    int fltype = !strncmp( identifier, "float", 5 );

    if( uint || sint || fltype ) {
      // Get size of the primitive
      unsigned char primitiveLen;
      if( uint ) {
        primitiveLen = 4;
      } else if( sint ) {
        primitiveLen = 3;
      } else {
        primitiveLen = 5;
      }

      if( primitiveLen == strLen ) {
        // You f'd up.
        printf( "primitive type requires width.\n" );
        return NULL;
      }

      size_t remainder = strLen - primitiveLen;
      char copy[ remainder + 1 ];
      memcpy( copy, identifier + primitiveLen, remainder );
      copy[ remainder ] = 0;

      unsigned long bitDepth = strtoul( copy, NULL, 10 );
      if( !bitDepth ) {
        printf( "primitive type requires numeric bit depth or bit depth greater than zero.\n" );
        return NULL;
      }

      if( uint || sint ) {
        result = gsCreateToken( uint ? UINT_TYPE : INT_TYPE );
      } else {
        result = gsCreateToken( FLOAT_TYPE );
      }

      result->data.literal.asInteger = bitDepth;
    } else {
      // If we get here...you got an identifier.
      result = gsCreateToken( IDENTIFIER );
      result->data.literal.asString = calloc( strLen + 1, sizeof( char ) );
      memcpy( result->data.literal.asString, identifier, strLen );
    }

  }

  return result;
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
        gsLexerIncrement( self );
        break;
      }
      case ')': {
        current = gsCreateToken( RIGHT_PAREN );
        gsLexerIncrement( self );
        break;
      }
      case '{': {
        current = gsCreateToken( LEFT_BRACE );
        gsLexerIncrement( self );
        break;
      }
      case '}': {
        current = gsCreateToken( RIGHT_BRACE );
        gsLexerIncrement( self );
        break;
      }
      case ',': {
        current = gsCreateToken( COMMA );
        gsLexerIncrement( self );
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
          gsLexerIncrement( self );
        }
        break;
      }
      case '-': {
        current = gsCreateToken( MINUS );
        gsLexerIncrement( self );
        break;
      }
      case '+': {
        current = gsCreateToken( PLUS );
        gsLexerIncrement( self );
        break;
      }
      case ';': {
        current = gsCreateToken( SEMICOLON );
        gsLexerIncrement( self );
        break;
      }
      case '*': {
        current = gsCreateToken( STAR );
        gsLexerIncrement( self );
        break;
      }
      case '%': {
        current = gsCreateToken( MODULO );
        gsLexerIncrement( self );
        break;
      }
      case '^': {
        current = gsCreateToken( BITWISE_XOR );
        gsLexerIncrement( self );
        break;
      }
      case '~': {
        current = gsCreateToken( ONES_COMPLIMENT );
        gsLexerIncrement( self );
        break;
      }
      case '[': {
        current = gsCreateToken( LEFT_BRACKET );
        gsLexerIncrement( self );
        break;
      }
      case ']': {
        current = gsCreateToken( RIGHT_BRACKET );
        gsLexerIncrement( self );
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
            gsLexerIncrement( self );
            break;
          }
          case '<': {
            current = gsCreateToken( LEFT_SHIFT );
            gsLexerIncrement( self );
            break;
          }
          default: {
            current = gsCreateToken( LESS );
            break;
          }
        }

        gsLexerIncrement( self );
        break;
      }
      case '>': {
        switch( *( self->currentCharacter + 1 ) ) {
          case '=': {
            current = gsCreateToken( GREATER_EQUAL );
            gsLexerIncrement( self );
            break;
          }
          case '>': {
            current = gsCreateToken( RIGHT_SHIFT );
            gsLexerIncrement( self );
            break;
          }
          default: {
            current = gsCreateToken( GREATER );
            break;
          }
        }

        gsLexerIncrement( self );
        break;
      }
      case '/': {
        if( *( self->currentCharacter + 1 ) == '/' ) {
          // Single-line comment
          // Advance currentCharacter/column to nearest 0 or \n
          while( *self->currentCharacter && *self->currentCharacter != '\n' ) {
            gsLexerIncrement( self );
          }

          continue;
        } else {
          current = gsCreateToken( SLASH );
          gsLexerIncrement( self );
        }
        break;
      }
      case ' ':
      case '\r':
      case '\t': {
        gsLexerIncrement( self );
        continue;
      }
      case '\n': {
        gsLexerIncrement( self );
        continue;
      }
      case '"': {
        size_t stringSize = 0;

        gsLexerIncrement( self );

        const char* begin = self->currentCharacter;
        while( *self->currentCharacter && *self->currentCharacter != '"' ) {
          stringSize++;
          gsLexerIncrement( self );
        }

        // Eat the last "
        gsLexerIncrement( self );

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
            gsLexerIncrement( self );
            stringSize++;
          }

          char value[ stringSize + 1 ];
          strncpy( value, begin, stringSize );
          value[ stringSize ] = 0;

          current = gsGetReservedWordOrIdentifier( value, stringSize );
          if( !current ) {
            // Invalid token after uint, int, or float type
            continue;
          }
        } else {
          printf( "Unexpected character at (%d, %d): %c", self->line, self->column, *self->currentCharacter );
          gsLexerIncrement( self );
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
  }

  return result;
}

const char* gsGetDebugOutput( List_Token* token ) {
  switch( token->data.type ) {
    case LEFT_PAREN:
      return "(";
    case RIGHT_PAREN:
      return ")";
    case LEFT_BRACE:
      return "{";
    case RIGHT_BRACE:
      return "}";
    case COMMA:
      return ",";
    case DOT:
      return ".";
    case MINUS:
      return "-";
    case PLUS:
      return "+";
    case SEMICOLON:
      return ";";
    case SLASH:
      return "/";
    case STAR:
      return "*";
    case MODULO:
      return "%";
    case BITWISE_XOR:
      return "^";
    case ONES_COMPLIMENT:
      return "~";
    case LEFT_BRACKET:
      return "[";
    case RIGHT_BRACKET:
      return "]";
    case BANG:
      return "!";
    case BANG_EQUAL:
      return "!=";
    case EQUAL:
      return "=";
    case EQUAL_EQUAL:
      return "==";
    case GREATER:
      return ">";
    case GREATER_EQUAL:
      return ">=";
    case RIGHT_SHIFT:
      return ">>";
    case LESS:
      return "<";
    case LESS_EQUAL:
      return "<=";
    case LEFT_SHIFT:
      return "<<";
    case AND:
      return "&&";
    case BITWISE_AND:
      return "&";
    case OR:
      return "||";
    case BITWISE_OR:
      return "|";
    case IDENTIFIER:
      return "<identifier>";
    case STRING:
      return "<string>";
    case INTEGER:
      return "<integer>";
    case REAL:
      return "<real>";
    case UINT_TYPE:
      return "<uint>";
    case INT_TYPE:
      return "<int>";
    case FLOAT_TYPE:
      return "<float>";
    case CLASS:
      return "class";
    case ELSE:
      return "else";
    case BOOL_FALSE:
      return "false";
    case BOOL_TRUE:
      return "true";
    case FOR:
      return "for";
    case IF:
      return "if";
    case NULL_TOKEN:
      return "null";
    case RETURN:
      return "return";
    case SUPER:
      return "super";
    case THIS:
      return "this";
    case VAR:
      return "var";
    case WHILE:
      return "while";
    case STATIC:
      return "static";
    case BOOL:
      return "bool";
    case ADDR:
      return "addr";
    case PACKAGE:
      return "package";
    case IMPORT:
      return "import";
    case FROM:
      return "from";
    default:
      return "<unknown>";
  }
}
