#ifndef GOLDSCORPION_LEX
#define GOLDSCORPION_LEX

#include "utility.h"
#include <stdbool.h>

typedef struct Lexer {
  char* buffer;
  char* currentCharacter;
  unsigned int line;
  unsigned int column;
} Lexer;

typedef union Literal {
  char* asString;
  long asInteger;
  double asDouble;
} Literal;

typedef enum TokenType {
  // Single-character tokens
  LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
  COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,
  MODULO, BITWISE_XOR, ONES_COMPLIMENT,

  // Single or double-character tokens
  BANG, BANG_EQUAL,
  EQUAL, EQUAL_EQUAL,
  GREATER, GREATER_EQUAL, RIGHT_SHIFT,
  LESS, LESS_EQUAL, LEFT_SHIFT,
  AND, BITWISE_AND,
  OR, BITWISE_OR,

  // Literals
  IDENTIFIER, STRING, INTEGER, REAL,

  // Keywords
  CLASS, ELSE, BOOL_FALSE, BOOL_TRUE, FOR, IF, NULL_TOKEN,
  RETURN, SUPER, THIS, VAR, WHILE,

  END_OF_FILE
} TokenType;

typedef struct Token {
  TokenType type;
  Literal literal;
} Token;

bool gsIsAlpha( char c );
bool gsIsAlphanumeric( char c );

List* gsGetReservedWordOrIdentifier( char* identifier );

List* gsProcessNumeric( Lexer* self );

List* gsCreateToken( TokenType type );

Lexer* gsOpenLexer( char* buffer );

Lexer* gsGetLexerFromFile( const char* filename );

List* gsLex( Lexer* self );
void gsCloseLexer( Lexer* self );
List* gsPeekSet( Lexer* self, char check, TokenType ifTrue, TokenType ifFalse );

#endif
