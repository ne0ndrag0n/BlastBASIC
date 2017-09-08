#ifndef GOLDSCORPION_LEX
#define GOLDSCORPION_LEX

#include <stdbool.h>
#include <stddef.h>

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
  MODULO, BITWISE_XOR, ONES_COMPLIMENT, LEFT_BRACKET, RIGHT_BRACKET,

  // Single or double-character tokens
  BANG, BANG_EQUAL,
  EQUAL, EQUAL_EQUAL,
  GREATER, GREATER_EQUAL, RIGHT_SHIFT,
  LESS, LESS_EQUAL, LEFT_SHIFT,
  AND, BITWISE_AND,
  OR, BITWISE_OR,

  // Literals
  IDENTIFIER, STRING, INTEGER, REAL,
  UINT_TYPE, INT_TYPE, FLOAT_TYPE,

  // Keywords
  CLASS, ELSE, BOOL_FALSE, BOOL_TRUE, FOR, IF, NULL_TOKEN,
  RETURN, SUPER, THIS, VAR, WHILE, STATIC, BOOL, ADDR,
  PACKAGE, IMPORT, FROM, NEW
} TokenType;

typedef struct Token {
  TokenType type;
  Literal literal;
} Token;

typedef struct List_Token {
  Token data;
  struct List_Token* next;
} List_Token;

bool gsIsAlpha( char c );
bool gsIsNumeric( char c );
bool gsIsAlphanumeric( char c );

void gsLexerIncrement( Lexer* self );

List_Token* gsGetReservedWordOrIdentifier( char* identifier, size_t strLen );

List_Token* gsProcessNumeric( Lexer* self );

List_Token* gsCreateToken( TokenType type );

Lexer* gsOpenLexer( char* buffer );

Lexer* gsGetLexerFromFile( const char* filename );

List_Token* gsLex( Lexer* self );
void gsCloseLexer( Lexer* self );
List_Token* gsPeekSet( Lexer* self, char check, TokenType ifTrue, TokenType ifFalse );

const char* gsGetDebugOutput( List_Token* token );
const char* gsTokenToString( TokenType type );

#endif
