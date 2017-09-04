#include "parse.h"
#include "lex.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PRIMARY_TOKENS_COUNT 8
static const TokenType PRIMARY_TOKENS[ PRIMARY_TOKENS_COUNT ] = { BOOL_TRUE, BOOL_FALSE, NULL_TOKEN, THIS, INTEGER, REAL, STRING, IDENTIFIER };
static int INDENTATION = -1;

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

/**
 * Get the first token *after* an alternating sequence of two token types
 */
List_Token* gsIndeterminateLookahead( List_Token* start, TokenType first, TokenType second ) {
  List_Token* current = start;

  while( true ) {
    if( !current || ( current->data.type != first ) ) {
      return current;
    }

    current = current->next;
    if( !current || ( current->data.type != second ) ) {
      return current;
    }

    current = current->next;
  }
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
  }

  return expr;
}

ASTNode* gsGetExpressionUnary( Parser* self ) {
  List_Token* match = NULL;

  if( ( match = gsParserExpect( self, BANG ) ) || ( match = gsParserExpect( self, MINUS ) ) ) {
    return gsCreateUnaryExpressionNode( match->data, gsGetExpressionUnary( self ) );
  } else if( ( match = gsParserExpect( self, NEW ) ) ) {
    List_Token* stack = gsParserExpect( self, STACK );
    return gsCreateUnaryExpressionNode( ( stack ? stack->data : match->data ), gsGetExpressionUnary( self ) );
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
    expr = gsCreateBinaryExpressionNode( expr, match->data, gsGetExpressionAssignment( self ) );
    expr->type = ASTAssignment;
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
      stmt->data.node = identifier;

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

ASTNode* gsGetReturnStatement( Parser* self ) {
  ASTNode* expr;

  if( gsParserExpect( self, SEMICOLON ) ) {
    // Return with no expression
    expr = NULL;
  } else {
    expr = gsGetExpression( self );

    if( !gsParserExpect( self, SEMICOLON ) ) {
      gsParserThrow( self, "Expected: ';' token after return statement" );
    }
  }

  ASTNode* result = calloc( 1, sizeof( ASTNode ) );
  result->type = ASTReturnStatement;
  result->data.node = expr;

  return result;
}

ASTNode* gsGetStatement( Parser* self ) {
  List_Token* match = NULL;

  if( ( match = gsParserExpect( self, PACKAGE ) ) ) {
    return gsGetPackageStatement( self );
  }

  if( ( match = gsParserExpect( self, IMPORT ) ) ) {
    return gsGetImportStatement( self );
  }

  if( ( match = gsParserExpect( self, RETURN ) ) ) {
    return gsGetReturnStatement( self );
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

ASTNode* gsGetVarDecl( Parser* self, bool independent ) {
  List_Token* match = NULL;
  ASTNode* type = NULL;
  ASTNode* identifier = NULL;
  ASTNode* expr = NULL;

  type = gsGetTypeSpecifier( self );

  // Get the identifier
  if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {
    identifier = gsCreatePrimaryNode( match->data );
  } else {
    gsParserThrow( self, "Expected: <identifier> token after type-specifier" );
  }

  if( independent ) {
    // Independent means check for an assignment (if present), and expect a semicolon.
    if( gsParserExpect( self, EQUAL ) ) {
      expr = gsGetExpression( self );
    }

    if( !gsParserExpect( self, SEMICOLON ) ) {
      gsParserThrow( self, "Expected: ';' after vardecl statement" );
    }
  }

  ASTNode* result = calloc( 1, sizeof( ASTNode ) );
  result->type = ASTVardecl;

  result->data.vardecl.typeSpecifier = type;
  result->data.vardecl.identifier = identifier;
  result->data.vardecl.assignmentExpression = expr;

  return result;
}

ASTNode* gsGetFunDecl( Parser* self ) {
  List_Token* match = NULL;

  ASTNode* type = gsGetTypeSpecifier( self );

  // Get the identifier
  ASTNode* identifier = NULL;
  if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {
    identifier = gsCreatePrimaryNode( match->data );
  } else {
    gsParserThrow( self, "Expected: <identifier> token after type-specifier" );
  }

  if( !gsParserExpect( self, LEFT_PAREN ) ) {
    gsParserThrow( self, "Expected: '(' token after <identifier> in function declaration" );
  }

  List_Node* arguments = NULL;
  List_Node* prev = NULL;
  bool quit = gsParserExpect( self, RIGHT_PAREN );
  while( !quit ) {
    // There's a vardecl to be processed
    if( !prev ) {
      arguments = calloc( 1, sizeof( List_Node ) );
      arguments->data = gsGetVarDecl( self, false );

      prev = arguments;
    } else {
      prev->next = calloc( 1, sizeof( List_Node ) );
      prev->next->data = gsGetVarDecl( self, false );
      prev = prev->next;
    }

    if( gsParserExpect( self, COMMA ) ) {
      // Ate a comma, don't quit.
      quit = false;
    } else if( gsParserExpect( self, RIGHT_PAREN ) ) {
      // Ate a ), quit!
      quit = true;
    } else {
      gsParserThrow( self, "Unexpected token parsing argument list for function declaration" );
    }
  }

  ASTNode* body = gsGetBlock( self );

  ASTNode* result = calloc( 1, sizeof( ASTNode ) );
  result->type = ASTFunction;
  result->data.function.typeSpecifier = type;
  result->data.function.identifier = identifier;
  result->data.function.arguments = arguments;
  result->data.function.body = body;

  return result;
}

ASTNode* gsGetDeclaration( Parser* self ) {
  // EZ PZ if the vardecl starts with a primitive type
  if(
    self->current &&
    (
      self->current->data.type == UINT_TYPE ||
      self->current->data.type == INT_TYPE ||
      self->current->data.type == FLOAT_TYPE ||
      self->current->data.type == BOOL ||
      self->current->data.type == VAR ||
      self->current->data.type == ADDR
    )
  ) {
    // Peek ahead to see if this is a fundecl

    // Should be on the identifier token here
    List_Token* tokenAfter = self->current->next;

    // The one after the identifier - if it's a (, we got a fundecl!
    if( tokenAfter && tokenAfter->next && tokenAfter->next->data.type == LEFT_PAREN ) {
      return gsGetFunDecl( self );
    }

    return gsGetVarDecl( self, true );
  }

  // Still looking for something beginning with a primitive-type. Check for identifier, infinite lookahead past ( DOT IDENTIFIER )*, and then for the token to be stopped at another IDENTIFIER
  if( self->current && self->current->data.type == IDENTIFIER ) {
    List_Token* tokenAfter = self->current->next;

    if( tokenAfter && tokenAfter->data.type == DOT ) {
      // Get this shit out of the way
      tokenAfter = gsIndeterminateLookahead( tokenAfter, DOT, IDENTIFIER );
    }

    if( tokenAfter && tokenAfter->data.type == LEFT_BRACKET ) {
      // Get this shit out of the way
      tokenAfter = tokenAfter->next;
      // Now should be on RIGHT_BRACKET but really we don't care when looking ahead
      if( tokenAfter ) {
        tokenAfter = tokenAfter->next;
        // Now should be on the token after what RIGHT_BRACKET should be
      }
    }

    if( tokenAfter && tokenAfter->data.type == IDENTIFIER ) {
      // So, by the time we get here, it's either a vardecl or a fundecl
      // Go one more up to look for a LEFT_PAREN and signal the fundecl
      tokenAfter = tokenAfter->next;

      if( tokenAfter && tokenAfter->data.type == LEFT_PAREN ) {
        return gsGetFunDecl( self );
      } else {
        return gsGetVarDecl( self, true );
      }
    }

  }

  return gsGetStatement( self );
}

ASTNode* gsGetProgram( Parser* self ) {
  List_Node* list = NULL;

  List_Node* prev = NULL;
  while( self->current ) {

    if( setjmp( self->exceptionHandler ) ) {
      printf( "Compile error: %s\n", self->error );
      // TODO: Tokens know where they are in the file (go back to the lexer and add this to each Token)
      //printf( "At %d,%d\n", 0, 0 );
      exit( 1 );
    } else {

      if( !prev ) {
        list = calloc( 1, sizeof( List_Node ) );
        list->data = gsGetDeclaration( self );

        prev = list;
      } else {
        prev->next = calloc( 1, sizeof( List_Node ) );
        prev->next->data = gsGetDeclaration( self );
        prev = prev->next;
      }

    }

  }

  ASTNode* result = calloc( 1, sizeof( ASTNode ) );
  result->type = ASTProgram;
  result->data.expressionList = list;

  return result;
}

ASTNode* gsGetBlock( Parser* self ) {
  List_Token* match = NULL;
  List_Node* declarations = NULL;

  if( ( match = gsParserExpect( self, LEFT_BRACE ) ) ) {

    List_Node* prev = NULL;
    while( !gsParserExpect( self, RIGHT_BRACE ) ) {
      if( !prev ) {
        declarations = calloc( 1, sizeof( List_Node ) );
        declarations->data = gsGetDeclaration( self );

        prev = declarations;
      } else {
        prev->next = calloc( 1, sizeof( List_Node ) );
        prev->next->data = gsGetDeclaration( self );
        prev = prev->next;
      }
    }

    // gsParserExpect can return NULL if we hit the end of the token list, not just matching RIGHT_BRACE
    if( self->prev->data.type == RIGHT_BRACE ) {
      ASTNode* result = calloc( 1, sizeof( ASTNode ) );
      result->type = ASTBlock;
      result->data.expressionList = declarations;

      return result;
    } else {
      gsParserThrow( self, "Expected: '}' token to close block" );
    }
  }

  // Shut up lint
  return NULL;
}

ASTNode* gsGetTypeSpecifier( Parser* self ) {
  List_Token* match = NULL;
  ASTNode* udt = NULL;
  bool array = false;
  Token primitive;

  // Get the type of this vardecl
  if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {
    udt = gsCreatePrimaryNode( match->data );

    while( ( match = gsParserExpect( self, DOT ) ) ) {
      if( ( match = gsParserExpect( self, IDENTIFIER ) ) ) {
        udt = gsCreateGetNode( udt, match->data.literal.asString );
      } else {
        gsParserThrow( self, "Expected: <identifier> token after '.' token" );
      }
    }
  } else if(
    ( match = gsParserExpect( self, UINT_TYPE ) ) ||
    ( match = gsParserExpect( self, INT_TYPE ) ) ||
    ( match = gsParserExpect( self, FLOAT_TYPE ) ) ||
    ( match = gsParserExpect( self, BOOL ) ) ||
    ( match = gsParserExpect( self, VAR ) ) ||
    ( match = gsParserExpect( self, ADDR ) )
  ) {
    primitive = match->data;
  } else {
    gsParserThrow( self, "Expected: primitive type or <identifier> token in type-specifier" );
  }

  // Is it an array type?
  if( ( match = gsParserExpect( self, LEFT_BRACKET ) ) ) {
    if( ( match = gsParserExpect( self, RIGHT_BRACKET ) ) ) {
      array = true;
    } else {
      gsParserThrow( self, "Expected: ']' after '[' token" );
    }
  }

  ASTNode* result = calloc( 1, sizeof( ASTNode ) );
  result->type = ASTTypeSpecifier;

  if( udt ) {
    result->data.specifier.udt = true;
    result->data.specifier.type.udt = udt;
  } else {
    result->data.specifier.udt = false;
    result->data.specifier.type.primitive = primitive;
  }

  result->data.specifier.array = array;

  return result;
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

/**
 * Print an abstract syntax tree node and the addresses of its children
 * Giant mess, don't give a shit
 */
void gsDebugPrintAST( ASTNode* root ) {
  int indentation = ++INDENTATION;

  char* description = NULL;

  switch( root->type ) {
    case ASTLiteral:
      description = "Literal";
      break;
    case ASTIdentifier:
      description = "Identifier";
      break;
    case ASTGetter:
      description = "Getter";
      break;
    case ASTArgumentList:
      description = "ArgumentList";
      break;
    case ASTCall:
      description = "Call";
      break;
    case ASTUnaryExpression:
      description = "UnaryExpression";
      break;
    case ASTBinaryExpression:
      description = "BinaryExpression";
      break;
    case ASTAssignment:
      description = "AssignmentExpression";
      break;
    case ASTPackageStatement:
      description = "PackageStatement";
      break;
    case ASTImportStatement:
      description = "ImportStatement";
      break;
    case ASTVardecl:
      description = "Vardecl";
      break;
    case ASTBlock:
      description = "Block";
      break;
    case ASTTypeSpecifier:
      description = "TypeSpecifier";
      break;
    case ASTFunction:
      description = "Function";
      break;
    case ASTReturnStatement:
      description = "ReturnStatement";
      break;
    case ASTProgram:
      description = "Program";
      break;
    default:
      description = "<unknown>";
  }

  gsParseOutputIndentation( indentation );
  printf( "%p::%s\n", root, description );

  switch( root->type ) {
    case ASTLiteral: {
      gsParseOutputIndentation( indentation );

      if( root->data.token.type == INTEGER ) {
        printf( "Integer Literal (%ld)\n", root->data.token.literal.asInteger );
      } else if( root->data.token.type == STRING ) {
        printf( "String Literal (%s)\n", root->data.token.literal.asString );
      } else if( root->data.token.type == REAL ) {
        printf( "Real Literal (%f)\n", root->data.token.literal.asDouble );
      } else if( root->data.token.type == NULL_TOKEN ) {
        printf( "Null Literal\n" );
      } else if( root->data.token.type == BOOL_TRUE ) {
        printf( "True Literal\n" );
      } else if( root->data.token.type == BOOL_FALSE ) {
        printf( "False Literal\n" );
      }
      break;
    }
    case ASTIdentifier: {
      gsParseOutputIndentation( indentation );

      if( root->data.token.type == IDENTIFIER ) {
        printf( "Identifier (%s)\n", root->data.token.literal.asString );
      } else if( root->data.token.type == THIS ) {
        printf( "'this' keyword\n" );
      }
      break;
    }
    case ASTGetter: {
      gsParseOutputIndentation( indentation );
      printf( "Field: %s, From:\n", ( root->data.get.field ? root->data.get.field : "" ) );
      if( root->data.get.source ) {
        gsDebugPrintAST( root->data.get.source );
      }
      break;
    }
    case ASTArgumentList:
    case ASTBlock:
    case ASTProgram: {
      List_Node* current = root->data.expressionList;
      while( current ) {
        gsDebugPrintAST( current->data );
        current = current->next;
      }
      break;
    }
    case ASTCall: {
      if( root->data.call.source ) {
        gsParseOutputIndentation( indentation );
        printf( "Callee:\n" );
        gsDebugPrintAST( root->data.call.source );
      }

      if( root->data.call.arguments ) {
        gsParseOutputIndentation( indentation );
        printf( "Arguments:\n" );
        gsDebugPrintAST( root->data.call.arguments );
      }

      break;
    }
    case ASTUnaryExpression: {
      gsParseOutputIndentation( indentation );
      printf( "op: %s\n", gsTokenToString( root->data.unaryExpression.op.type ) );

      if( root->data.unaryExpression.rhs ) {
        gsParseOutputIndentation( indentation );
        printf( "rhs:\n" );
        gsDebugPrintAST( root->data.unaryExpression.rhs );
      }
      break;
    }
    case ASTBinaryExpression:
    case ASTAssignment: {
      gsParseOutputIndentation( indentation );
      printf( "op: %s\n", gsTokenToString( root->data.binaryExpression.op.type ) );

      if( root->data.binaryExpression.lhs ) {
        gsParseOutputIndentation( indentation );
        printf( "lhs:\n" );
        gsDebugPrintAST( root->data.binaryExpression.lhs );
      }

      if( root->data.binaryExpression.rhs ) {
        gsParseOutputIndentation( indentation );
        printf( "rhs:\n" );
        gsDebugPrintAST( root->data.binaryExpression.rhs );
      }
      break;
    }
    case ASTPackageStatement:
    case ASTReturnStatement: {
      gsParseOutputIndentation( indentation );
      printf( "Identifier:\n" );

      if( root->data.node ) {
        gsDebugPrintAST( root->data.node );
      }
      break;
    }
    case ASTImportStatement: {
      List_Node* current = root->data.import.imports;
      while( current ) {
        gsParseOutputIndentation( indentation );
        printf( "Import:\n" );
        gsDebugPrintAST( current->data );
        current = current->next;
      }

      gsParseOutputIndentation( indentation );
      printf( "From:\n" );
      if( root->data.import.from ) {
        gsDebugPrintAST( root->data.import.from );
      }
      break;
    }
    case ASTVardecl: {
      if( root->data.vardecl.typeSpecifier ) {
        gsParseOutputIndentation( indentation );
        printf( "Type specifier:\n" );
        gsDebugPrintAST( root->data.vardecl.typeSpecifier );
      }

      if( root->data.vardecl.identifier ) {
        gsParseOutputIndentation( indentation );
        printf( "Identifier:\n" );
        gsDebugPrintAST( root->data.vardecl.identifier );
      }

      if( root->data.vardecl.assignmentExpression ) {
        gsParseOutputIndentation( indentation );
        printf( "Assignment:\n" );
        gsDebugPrintAST( root->data.vardecl.assignmentExpression );
      }

      break;
    }
    case ASTTypeSpecifier: {
      gsParseOutputIndentation( indentation );
      printf( "udt: %d array: %d\n", root->data.specifier.udt, root->data.specifier.array );

      if( root->data.specifier.udt ) {
        gsParseOutputIndentation( indentation );
        printf( "udt:\n" );
        gsDebugPrintAST( root->data.specifier.type.udt );
      } else {
        gsParseOutputIndentation( indentation );
        printf( "primitive: %s\n", gsTokenToString( root->data.specifier.type.primitive.type ) );
      }
      break;
    }
    case ASTFunction: {
      if( root->data.function.typeSpecifier ) {
        gsParseOutputIndentation( indentation );
        printf( "Type Specifier:\n" );
        gsDebugPrintAST( root->data.function.typeSpecifier );
      }

      if( root->data.function.identifier ) {
        gsParseOutputIndentation( indentation );
        printf( "Identifier:\n" );
        gsDebugPrintAST( root->data.function.identifier );
      }

      if( root->data.function.body ) {
        gsParseOutputIndentation( indentation );
        printf( "Body:\n" );
        gsDebugPrintAST( root->data.function.body );
      }

      List_Node* current = root->data.function.arguments;
      while( current ) {
        gsParseOutputIndentation( indentation );
        printf( "Argument:\n" );
        gsDebugPrintAST( current->data );
        current = current->next;
      }

      break;
    }
  }

  --INDENTATION;
}

void gsParseOutputIndentation( int indentation ) {
  for( int i = 0; i < indentation; i++ ) {
    printf( "-" );
  }
}
