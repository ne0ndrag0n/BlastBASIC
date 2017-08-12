#ifndef GOLDSCORPION_LEX
#define GOLDSCORPION_LEX

#include "utility.h"

typedef struct Lexer {
  char* buffer;
  char* currentCharacter;
  unsigned int line;
  unsigned int column;
} Lexer;

typedef union Literal {
  const char* asString;
  long asNumber;
  double asDouble;
} Literal;

typedef enum TokenType {
  // Single-character tokens
  LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
  COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

  // Single or double-character tokens
  BANG, BANG_EQUAL,
  EQUAL, EQUAL_EQUAL,
  GREATER, GREATER_EQUAL,
  LESS, LESS_EQUAL,

  // Literals
  IDENTIFIER, STRING, NUMBER,

  // Keywords
  AND, CLASS, ELSE, BOOL_FALSE, FUN, FOR, IF, NIL, OR,
  PRINT, RETURN, SUPER, THIS, BOOL_TRUE, VAR, WHILE,

  END_OF_FILE
} TokenType;

typedef struct Token {
  TokenType type;
  Literal literal;
  unsigned int line;
} Token;

List* gsCreateToken( TokenType type );

Lexer* gsOpenLexer( char* buffer );

Lexer* gsGetLexerFromFile( const char* filename );

List* gsLex( Lexer* self );
void gsCloseLexer( Lexer* self );
List* gsPeekSet( Lexer* self, char check, TokenType ifTrue, TokenType ifFalse );

#endif
