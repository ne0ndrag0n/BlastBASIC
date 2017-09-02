#ifndef GOLDSCORPION_PARSE
#define GOLDSCORPION_PARSE

#include <stddef.h>
#include <setjmp.h>
#include <stdbool.h>
#include "lex.h"

struct ASTNode;

typedef enum ASTNodeType {
  ASTLiteral,
  ASTIdentifier,
  ASTGetter,
  ASTArgumentList,
  ASTCall,
  ASTUnaryExpression
} ASTNodeType;

typedef struct ExprGet {
  struct ASTNode* source;
  char* field;
} ExprGet;

typedef struct ExprCall {
  struct ASTNode* source;
  struct ASTNode* arguments;
} ExprCall;

typedef struct UnaryExpression {
  struct ASTNode* rhs;
  Token op;
} UnaryExpression;

typedef struct List_Expression {
  struct ASTNode* data;
  struct List_Expression* next;
} List_Expression;

typedef struct ASTNode {
  ASTNodeType type;
  union {
    // ASTLiteral, ASTIdentifier
    Token token;
    // ASTGetter
    ExprGet get;
    // ASTArgumentList
    List_Expression* expressionList;
    // ASTCall
    ExprCall call;
    // ASTUnaryExpression
    UnaryExpression unaryExpression;
  } data;
} ASTNode;

typedef struct Parser {
  List_Token* prev;
  List_Token* current;
  jmp_buf exceptionHandler;
  char* error;
} Parser;

void gsParserThrow( Parser* self, char* error );

void gsParserIncrement( Parser* self );
List_Token* gsParserExpect( Parser* self, TokenType type );

ASTNode* gsCreatePrimaryNode( Token token );
ASTNode* gsCreateGetNode( ASTNode* source, char* field );
ASTNode* gsCreateCallNode( ASTNode* source, ASTNode* arguments );

ASTNode* gsGetArguments( Parser* self );

ASTNode* gsGetExpressionPrimary( Parser* self );
ASTNode* gsGetExpressionCall( Parser* self );
ASTNode* gsGetExpressionUnary( Parser* self );
ASTNode* gsGetExpression( Parser* self );

Parser* gsGetParser( List_Token* starterToken );

#endif
