#ifndef GOLDSCORPION_LEX
#define GOLDSCORPION_LEX

#include "utility.h"

typedef struct Lexer {
  char* buffer;
  unsigned int line;
  unsigned int start;
  unsigned int current;
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
  const char* lexeme;
  Literal literal;
  unsigned int line;
} Token;

Token* gsCreateToken( TokenType type );

Lexer* gsOpenLexer( char* buffer );
void gsCloseLexer( Lexer* lexer );

Lexer* gsGetLexerFromFile( const char* filename );

List* gsLex( Lexer* lexer );

#endif
