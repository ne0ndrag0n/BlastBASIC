#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "lex.h"

int main( int argumentCount, char** arguments ) {
  char* files[ 256 ] = { 0 };
  size_t numFiles = 0;

  if( argumentCount > 1 ) {
    for( int i = 1; i < argumentCount; i++ ) {
      if( !strncmp( arguments[ i ], "--", 2 ) ) {
        if( !strcmp( arguments[ i ], "--help" ) ) {
          printf( "Usage: gs [options] file...\n" );
          printf( "Options:\n" );
          printf( "\t--target=<target>    Specify the system target. Target may be an arch or a specific embedded system.\n" );
          printf( "\t                     Supported targets: amd64, smd\n" );
          printf( "\n" );
          printf( "\t--help               Display this help screen.\n" );
          printf( "\t--version            Print GoldScorpion version.\n");
          printf( "\n" );
        } else {
          printf( "Unknown option: %s\n", arguments[ i ] );
        }
      } else {
        // Pile into files
        if( numFiles <= 255 ) {
          files[ numFiles ] = arguments[ i ];
          numFiles++;
        }
      }
    }
  } else {
    printf( "gs: please provide at least one .sco file, or specify --help for more options.\n" );
    return 1;
  }

  // Process files - 1 lexer per file
  Lexer* lexers[ numFiles ];
  for( size_t i = 0; i < numFiles; i++ ) {
    lexers[ i ] = gsGetLexerFromFile( files[ i ] );
    if( !lexers[ i ] ) {
      printf( "Skipping file %s\n", arguments[ i ] );
      continue;
    }

    List_Token* tokens = gsLex( lexers[ i ] );
    List_Token* current = tokens;
    while( current != NULL ) {
      printf( "Token: %s", gsGetDebugOutput( current ) );
      switch( current->data.type ) {
        case IDENTIFIER:
        case STRING: {
          printf( " (%s)", current->data.literal.asString );
          break;
        }
        case UINT_TYPE:
        case INT_TYPE:
        case FLOAT_TYPE: {
          printf( " (%ld-bit)", current->data.literal.asInteger );
          break;
        }
        case INTEGER: {
          printf( " (%ld)", current->data.literal.asInteger );
          break;
        }
        default:
          break;
      }
      printf( "\n" );
      current = current->next;
    }
  }

  return 0;
}
