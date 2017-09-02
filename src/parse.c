#include "parse.h"
#include "lex.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PRIMARY_TOKENS_COUNT 8
static const TokenType PRIMARY_TOKENS[ PRIMARY_TOKENS_COUNT ] = { BOOL_TRUE, BOOL_FALSE, NULL_TOKEN, THIS, INTEGER, REAL, STRING, IDENTIFIER };

void gsParserThrow( Parser* self, char* error ) {
  self->error = error;

  longjmp( self->exceptionHandler, 1 );
}

/**
 * If the current token type matches "type", return the token and increment the parser.
 * Otherwise, return a null pointer.
 */
List_Token* gsParserExpect( Parser* self, TokenType type ) {
  if( !self->current ) {
    return NULL;
  }

  if( self->current->data.type == type ) {
    gsParserIncrement( self );
    return self->prev;
  }

  return NULL;
}


ASTNode* gsCreatePrimaryNode( Token token ) {
  ASTNode* result = calloc( 1, sizeof( ASTNode ) );

  result->type = ( token.type == IDENTIFIER || token.type == THIS ) ? ASTIdentifier : ASTLiteral;
  result->data.token = token;

  // Deep copy all strings - lexer should be allowed to close itself after an AST is created
  if( token.type == IDENTIFIER || token.type == STRING ) {
    result->data.token.literal.asString = calloc( 1 + strlen( token.literal.asString ), sizeof( char ) );
    strcpy( result->data.token.literal.asString, token.literal.asString );
  }

  return result;
}

ASTNode* gsCreateGetNode( ASTNode* source, char* field ) {
  ASTNode* result = calloc( 1, sizeof( ASTNode ) );

  result->type = ASTGetter;
  result->data.get.source = source;
  result->data.get.field = calloc( 1 + strlen( field ), sizeof( char ) );
  strcpy( result->data.get.field, field );

  return result;
}

ASTNode* gsCreateCallNode( ASTNode* source, ASTNode* arguments ) {
  ASTNode* result = calloc( 1, sizeof( ASTNode ) );

  result->type = ASTCall;
  result->data.call.source = source;
  result->data.call.arguments = arguments;

  return result;
}

ASTNode* gsCreateUnaryExpressionNode( Token op, ASTNode* rhs ) {
  ASTNode* expr = calloc( 1, sizeof( ASTNode ) );
  expr->type = ASTUnaryExpression;

  expr->data.unaryExpression.op = op;
  expr->data.unaryExpression.rhs = rhs;

  return expr;
}

ASTNode* gsCreateBinaryExpressionNode( ASTNode* lhs, Token op, ASTNode* rhs ) {
  ASTNode* expr = calloc( 1, sizeof( ASTNode ) );
  expr->type = ASTBinaryExpression;

  expr->data.binaryExpression.lhs = lhs;
  expr->data.binaryExpression.op = op;
  expr->data.binaryExpression.rhs = rhs;

  return expr;
}

ASTNode* gsCreateAssignmentExpressionNode( ASTNode* lhs, Token op, ASTNode* rhs, bool nw, bool stack ) {
  ASTNode* expr = calloc( 1, sizeof( ASTNode ) );
  expr->type = ASTAssignment;

  expr->data.assignmentExpression.lhs = lhs;
  expr->data.assignmentExpression.op = op;
  expr->data.assignmentExpression.rhs = rhs;

  expr->data.assignmentExpression.newQualifier = nw;
  expr->data.assignmentExpression.stackQualifier = nw ? stack : false;

  return expr;
}

/**
 * TODO: if gsGetExpression throws, set local jmp_buf to clean up allocations. Then longjmp to original jmp_buf
 * Don't give a shit about leaks here for now because when the parser encounters an error, it'll close and the OS will deallocate it.
 *
 * Only call this if you have at least one argument (look ahead for the RIGHT_PAREN right after the LEFT_PAREN)
 */
ASTNode* gsGetArguments( Parser* self ) {
  ASTNode* result = calloc( 1, sizeof( ASTNode ) );
  result->type = ASTArgumentList;

  // Get first expression
  result->data.expressionList = calloc( 1, sizeof( List_Node ) );
  result->data.expressionList->data = gsGetExpression( self );

  List_Node* prev = result->data.expressionList;
  while( self->current && self->current->data.type == COMMA ) {
    // Eat the COMMA
    gsParserIncrement( self );

    prev->next = calloc( 1, sizeof( List_Node ) );
    prev->next->data = gsGetExpression( self );
    prev = prev->next;
  }

  return result;
}

ASTNode* gsGetExpressionPrimary( Parser* self ) {
  List_Token* match = NULL;

  for( size_t i = 0; i != PRIMARY_TOKENS_COUNT; i++ ) {
    match = gsParserExpect( self, PRIMARY_TOKENS[ i ] );
    if( match ) {
      return gsCreatePrimaryNode( match->data );
    }
  }

  if( ( match = gsParserExpect( self, LEFT_PAREN ) ) ) {
    ASTNode* expression = gsGetExpression( self );

    if( gsParserExpect( self, RIGHT_PAREN ) ) {
      // return the gsGetExpression result
      return expression;
    } else {
      gsParserThrow( self, "Expected: ) token" );
    }
  }

  gsParserThrow( self, "Expected: 'primary' or ( token" );

  // Shuts up lint
  return NULL;
}

ASTNode* gsGetExpressionCall( Parser* self ) {
  ASTNode* expr = gsGetExpressionPrimary( self );
  List_Token* match = NULL;

  while( self->current && ( self->current->data.type == LEFT_PAREN || self->current->data.type == DOT ) ) {
    // Don't even do anything else if the left hand side of these exprs are incompatible types
    if( expr->type == ASTIdentifier || expr->type == ASTCall || expr->type == ASTGetter ) {
      if( self->current->data.type == LEFT_PAREN ) {
        // Function call
        gsParserIncrement( self );

        // If the next token is a RIGHT_PAREN, there's no arguments.
        if( ( match = gsParserExpect( self, RIGHT_PAREN ) ) ) {
          expr = gsCreateCallNode( expr, NULL );
        } else {
          expr = gsCreateCallNode( expr, gsGetArguments( self ) );

          // Expect a RIGHT_PAREN
          if( !( match = gsParserExpect( self, RIGHT_PAREN ) ) ) {
            gsParserThrow( self, "Expected: ) token" );
          }
        }
      } else {
        // Getter
        gsParserIncrement( self );

        if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {
          expr = gsCreateGetNode( expr, match->data.literal.asString );
        } else {
          gsParserThrow( self, "Expected: <identifier> token" );
        }
      }
    } else {
      gsParserThrow( self, "Invalid left-hand side of '(' or '.' operator" );
    }
  }

  return expr;
}

ASTNode* gsGetExpressionUnary( Parser* self ) {
  List_Token* match = NULL;

  if( ( match = gsParserExpect( self, BANG ) ) || ( match = gsParserExpect( self, MINUS ) ) ) {
    return gsCreateUnaryExpressionNode( match->data, gsGetExpressionUnary( self ) );
  } else {
    return gsGetExpressionCall( self );
  }
}

ASTNode* gsGetExpressionMultiplication( Parser* self ) {
  ASTNode* expr = gsGetExpressionUnary( self );
  List_Token* match = NULL;

  while( ( match = gsParserExpect( self, SLASH ) ) || ( match = gsParserExpect( self, STAR ) ) ) {
    expr = gsCreateBinaryExpressionNode( expr, match->data, gsGetExpressionUnary( self ) );
  }

  return expr;
}

ASTNode* gsGetExpressionAddition( Parser* self ) {
  ASTNode* expr = gsGetExpressionMultiplication( self );
  List_Token* match = NULL;

  while( ( match = gsParserExpect( self, MINUS ) ) || ( match = gsParserExpect( self, PLUS ) ) ) {
    expr = gsCreateBinaryExpressionNode( expr, match->data, gsGetExpressionMultiplication( self ) );
  }

  return expr;
}

ASTNode* gsGetExpressionComparison( Parser* self ) {
  ASTNode* expr = gsGetExpressionAddition( self );
  List_Token* match = NULL;

  while(
    ( match = gsParserExpect( self, GREATER ) ) ||
    ( match = gsParserExpect( self, GREATER_EQUAL ) ) ||
    ( match = gsParserExpect( self, LESS ) ) ||
    ( match = gsParserExpect( self, LESS_EQUAL ) )
  ) {
    expr = gsCreateBinaryExpressionNode( expr, match->data, gsGetExpressionAddition( self ) );
  }

  return expr;
}

ASTNode* gsGetExpressionEquality( Parser* self ) {
  ASTNode* expr = gsGetExpressionComparison( self );
  List_Token* match = NULL;

  while( ( match = gsParserExpect( self, BANG_EQUAL ) ) || ( match = gsParserExpect( self, EQUAL_EQUAL ) ) ) {
    expr = gsCreateBinaryExpressionNode( expr, match->data, gsGetExpressionComparison( self ) );
  }

  return expr;
}

ASTNode* gsGetExpressionLogicAnd( Parser* self ) {
  ASTNode* expr = gsGetExpressionEquality( self );
  List_Token* match = NULL;

  while( ( match = gsParserExpect( self, AND ) ) ) {
    expr = gsCreateBinaryExpressionNode( expr, match->data, gsGetExpressionEquality( self ) );
  }

  return expr;
}

ASTNode* gsGetExpressionLogicOr( Parser* self ) {
  ASTNode* expr = gsGetExpressionLogicAnd( self );
  List_Token* match = NULL;

  while( ( match = gsParserExpect( self, OR ) ) ) {
    expr = gsCreateBinaryExpressionNode( expr, match->data, gsGetExpressionLogicAnd( self ) );
  }

  return expr;
}

ASTNode* gsGetExpressionAssignment( Parser* self ) {
  ASTNode* expr = gsGetExpressionLogicOr( self );
  List_Token* match = NULL;

  if( ( match = gsParserExpect( self, EQUAL ) ) ) {
    // expr, the left-hand side above, must be an ASTNode of type ASTIdentifier, ASTGetter
    if( expr->type == ASTIdentifier || expr->type == ASTGetter ) {
      bool nw = gsParserExpect( self, NEW ) ? true : false;
      bool stack = false;
      if( nw ) {
        stack = gsParserExpect( self, STACK ) ? true : false;
      }

      expr = gsCreateAssignmentExpressionNode( expr, match->data, gsGetExpressionAssignment( self ), nw, stack );
    } else {
      gsParserThrow( self, "Invalid left-hand side of assignment operator" );
    }
  }

  return expr;
}

ASTNode* gsGetExpression( Parser* self ) {
  return gsGetExpressionAssignment( self );
}

ASTNode* gsGetPackageStatement( Parser* self ) {
  List_Token* match = NULL;

  if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {
    ASTNode* identifier = gsCreatePrimaryNode( match->data );

    while( ( match = gsParserExpect( self, DOT ) ) ) {
      if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {
        identifier = gsCreateGetNode( identifier, match->data.literal.asString );
      } else {
        gsParserThrow( self, "Expected: <identifier> token after '.' token" );
      }
    }

    if( ( match = gsParserExpect( self, SEMICOLON ) ) ) {
      ASTNode* stmt = calloc( 1, sizeof( ASTNode ) );
      stmt->type = ASTPackageStatement;
      stmt->data.identifier = identifier;

      return stmt;
    } else {
      gsParserThrow( self, "Expected: ; after 'package' statement" );
    }
  } else {
    gsParserThrow( self, "Expected: <identifier> token after 'package' keyword" );
  }

  // Shut up lint
  return NULL;
}

ASTNode* gsGetImportStatement( Parser* self ) {
  List_Token* match = NULL;
  List_Node* imports = NULL;
  ASTNode* target = NULL;

  if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {

    imports = calloc( 1, sizeof( List_Node ) );
    imports->data = gsCreatePrimaryNode( match->data );

    List_Node* prev = imports;
    while( ( match = gsParserExpect( self, COMMA ) ) ) {
      if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {
        prev->next = calloc( 1, sizeof( List_Node ) );
        prev->next->data = gsCreatePrimaryNode( match->data );
        prev = prev->next;
      } else {
        gsParserThrow( self, "Expected: <identifier> token after ',' token" );
      }
    }

    if( ( match = gsParserExpect( self, FROM ) ) ) {

      if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {

        target = gsCreatePrimaryNode( match->data );
        while( ( match = gsParserExpect( self, DOT ) ) ) {

          if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {
            target = gsCreateGetNode( target, match->data.literal.asString );
          } else {
            gsParserThrow( self, "Expected: <identifier> token after '.' token" );
          }

        }

        if( ( match = gsParserExpect( self, SEMICOLON ) ) ) {
          ASTNode* result = calloc( 1, sizeof( ASTNode ) );
          result->type = ASTImportStatement;
          result->data.import.imports = imports;
          result->data.import.from = target;

          return result;
        } else {
          gsParserThrow( self, "Expected: ';' token after import statement" );
        }

      } else {
        gsParserThrow( self, "Expected: <identifier> token after 'from' keyword" );
      }

    } else {
      gsParserThrow( self, "Expected: 'from' keyword after <identifier> list" );
    }

  } else {
    gsParserThrow( self, "Expected: <identifier> token after 'import' keyword" );
  }

  // Shut up lint
  return NULL;
}

ASTNode* gsGetStatement( Parser* self ) {
  List_Token* match = NULL;

  if( ( match = gsParserExpect( self, PACKAGE ) ) ) {
    return gsGetPackageStatement( self );
  }

  if( ( match = gsParserExpect( self, IMPORT ) ) ) {
    return gsGetImportStatement( self );
  }

  ASTNode* expr = gsGetExpression( self );
  if( ( match = gsParserExpect( self, SEMICOLON ) ) ) {
    return expr;
  } else {
    gsParserThrow( self, "Expected: ; after expression statement" );
  }

  // Shut up lint
  return NULL;
}

void gsParserIncrement( Parser* self ) {
  self->prev = self->current;
  self->current = self->current->next;
}

Parser* gsGetParser( List_Token* starterToken ) {
  Parser* parser = calloc( 1, sizeof( Parser ) );

  parser->current = starterToken;

  return parser;
}
