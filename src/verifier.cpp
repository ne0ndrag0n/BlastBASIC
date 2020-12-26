#include "verifier.hpp"
#include "error.hpp"
#include "type_tools.hpp"
#include "tree_tools.hpp"
#include <variant>

namespace GoldScorpion {

    // Forward Declarations
    static void check( const Expression& node, MemoryTracker& memory );
    // end

    static void expectTokenString( const Token& token, const std::string& error ) {
        if( !token.value || !std::holds_alternative< std::string >( *token.value ) ) {
           Error{ error, token }.throwException();
        }
    }

    static void expectTokenLong( const Token& token, const std::string& error ) {
        if( !token.value || !std::holds_alternative< long >( *token.value ) ) {
           Error{ error, token }.throwException();
        }
    }

    static void expectTokenValue( const Token& token, const std::string& error ) {
        if( !token.value ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectTokenType( const Token& token, const std::string& error ) {
        if( token.type != TokenType::TOKEN_U8 ||
            token.type != TokenType::TOKEN_U16 ||
            token.type != TokenType::TOKEN_U32 ||
            token.type != TokenType::TOKEN_S8 ||
            token.type != TokenType::TOKEN_S16 ||
            token.type != TokenType::TOKEN_S32 ||
            token.type != TokenType::TOKEN_STRING ||
            token.type != TokenType::TOKEN_IDENTIFIER ||
            ( token.type == TokenType::TOKEN_IDENTIFIER && ( !token.value || !std::holds_alternative< std::string >( *token.value ) ) )
        ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectTokenOfType( const Token& token, const TokenType& type, const std::string& error ) {
        if( token.type != type ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectToken( const Primary& primary, std::optional< Token > nearestToken, const std::string& error ) {
        if( !std::holds_alternative< Token >( primary.value ) ) {
            Error{ error, nearestToken }.throwException();
        }
    }

    static void check( const BinaryExpression& node, std::optional< Token > nearestToken, MemoryTracker& memory ) {
        Error{ "Internal compiler error (BinaryExpression check not implemented)", {} }.throwException();
    }

    static void check( const AssignmentExpression& node, std::optional< Token > nearestToken, MemoryTracker& memory ) {
        // Left hand side must be either primary expression type with identifier, or binaryexpression type with dot operator
        const Expression& identifierExpression = *node.identifier;
        if( auto primaryExpression = std::get_if< std::unique_ptr< Primary > >( &identifierExpression.value ) ) {
            const Primary& primary = **primaryExpression;

            expectToken( primary, identifierExpression.nearestToken, "Expected: Primary expression in LHS of AssignmentExpression must be a single identifier" );

            // Primary expression must contain a token of type IDENTIFIER
            const Token& token = std::get< Token >( primary.value );
            expectTokenOfType( token, TokenType::TOKEN_IDENTIFIER, "Primary expression in LHS of AssignmentExpression must be a single identifier" );
            expectTokenString( token, "Internal compiler error (AssignmentExpression token has no string alternative" );

        } else if( auto result = std::get_if< std::unique_ptr< BinaryExpression > >( &identifierExpression.value ) ) {
            // Validate this binary expression
            const BinaryExpression& binaryExpression = **result;
            check( binaryExpression, identifierExpression.nearestToken, memory );

            // Validate this binary expression is a dot expression
            expectToken( *binaryExpression.op, identifierExpression.nearestToken, "Expected: Operator token of type ." );
        } else {
            Error{ "Invalid left-hand expression type for AssignmentExpression", nearestToken }.throwException();
        }

        // Right hand side must be valid expression
        check( *node.expression, memory );

        // Type of right hand side assignment should match type of identifier on left hand side
        std::optional< std::string > lhsType = getType( *node.identifier, memory );
        if( !lhsType ) {
            Error{ "Internal compiler error (AssignmentExpression unable to determine type for node.identifier)", nearestToken }.throwException();
        }

        std::optional< std::string > rhsType = getType( *node.expression, memory );
        if( !rhsType ) {
            Error{ "Internal compiler error (AssignmentExpression unable to determine type for node.expression", nearestToken }.throwException();
        }

        bool integerTypesMatch = typeIsInteger( *lhsType ) && typeIsInteger( *rhsType );
        bool typesMatch = *lhsType == *rhsType;
        if( !( integerTypesMatch || typesMatch ) ) {
            Error{ "Type mismatch: Expected type " + *lhsType + " but expression is of type " + *rhsType, nearestToken }.throwException();
        }
    }

    static void check( const Expression& node, MemoryTracker& memory ) {
        Error{ "Internal compiler error (Expression check not implemented)", {} }.throwException();
    }

    static void check( const VarDeclaration& node, MemoryTracker& memory ) {
        // def x as type = value

        // Verify x is an identifier containing a string
        expectTokenOfType( node.variable.name, TokenType::TOKEN_IDENTIFIER, "Expected: Identifier as token type for parameter declaration" );
        expectTokenString( node.variable.name, "Internal compiler error (VarDeclaration variable.name has no string alternative)" );

        // Verify type is either primitive or declared
        expectTokenType( node.variable.type.type, "Expected: User-defined type or one of [u8, u16, u32, s8, s16, s32, string]" );

        // If type is user-defined type (IDENTIFIER) then we must verify the UDT was declared
        if( node.variable.type.type.type == TokenType::TOKEN_IDENTIFIER ) {
            std::string typeId = std::get< std::string >( *node.variable.type.type.value );
            if( !memory.findUdt( typeId ) ) {
                Error{ "Undeclared user-defined type: " + typeId, node.variable.type.type }.throwException();
            }
        }

        // Use memory tracker to push a memory element
        std::optional< std::string > identifierTitle = getIdentifierName( node.variable.name );
        if( !identifierTitle ) {
            Error{ "Internal compiler error (VarDeclaration variable.name is not an identifier)", node.variable.name }.throwException();
        }

        std::optional< std::string > typeId = tokenToTypeId( node.variable.type.type );
        if( !typeId ) {
            Error{ "Internal compiler error (VarDeclaration variable.type should be properly verified)", node.variable.type.type }.throwException();
        }

        // The type returned by the expression on the right must match the declared type, or be coercible to the type.
        if( node.value ) {
            // Validate expression
            check( **node.value, memory );

            // Get type of expression
            std::optional< std::string > expressionType = getType( **node.value, memory );
            if( !expressionType ) {
                Error{ "Internal compiler error (VarDeclaration validated Expression failed to yield a type)", {} }.throwException();
            }

            // If both types are integer types then they are compatible with one another
            bool integerTypesMatch = typeIsInteger( *typeId ) && typeIsInteger( *expressionType );
            // Otherwise the types must match directly
            bool typesMatch = typeId == expressionType;

            if( !( typesMatch || integerTypesMatch ) ) {
                Error{ "Type mismatch: Expected type " + *typeId + " but expression is of type " + *expressionType, node.variable.type.type }.throwException();
            }
        }

        memory.push( MemoryElement { *identifierTitle, *typeId, 0, 0 } );
    }

    /**
     * Run a verification step to make sure items are logically consistent
     * Additionally, build a MemoryTracker object that will contain registered UDTs encountered
     */
    Result< MemoryTracker > check( const Program& program ) {
        return "Not implemented";
    }

}