#ifndef GS_LEXER
#define GS_LEXER

typedef enum TokenType {
  NONE,

  // Single character tokens
  LEFT_PAREN, RIGHT_PAREN,
  LEFT_BRACKET, RIGHT_BRACKET,
  DOT, MINUS, PLUS, SLASH, STAR,
  MODULO, BACKSLASH,

  // Single or double-character tokens
  EQUAL, EQUAL_EQUAL,
  GREATER, GREATER_EQUAL, RIGHT_SHIFT,
  LESS, LESS_EQUAL, LEFT_SHIFT,
  AND_SYMBOL, OR_SYMBOL,

  // Literals
  IDENTIFIER, STRING_TYPE, INTEGER, REAL,
  UINT_TYPE, INT_TYPE, REAL_TYPE,

  // Keywords
  CLASS, ELSE, BOOL_FALSE, BOOL_TRUE, FOR, IF, NULL_TOKEN, FROM,
  RETURN, SUPER, THIS, DEF, WHILE, BOOL, STRING, PACKAGE, IMPORT,
  FUNCTION

} TokenType;

typedef union Literal {
  char* asString;
  long asInteger;
  double asReal;
} Literal;

typedef struct Token {
  TokenType type;
  Literal literal;
} Token;

typedef struct List_Token {
  Token data;
  struct List_Token* next;
} List_Token;

List_Token* gsTokenizeFile( char* filename );

#endif
