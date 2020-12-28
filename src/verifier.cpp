#include "verifier.hpp"
#include "error.hpp"
#include "type_tools.hpp"
#include "tree_tools.hpp"
#include "variant_visitor.hpp"
#include <variant>
#include <set>

namespace GoldScorpion {

    // Forward Declarations
    static void check( const Expression& node, MemoryTracker& memory );
    // end

    static std::string expectTokenString( const Token& token, const std::string& error ) {
        if( token.value ) {
            if( auto stringValue = std::get_if< std::string >( &*token.value ) ) {
                return *stringValue;
            }
        }

        Error{ error, token }.throwException();
        return "";
    }

    static long expectTokenLong( const Token& token, const std::string& error ) {
        if( token.value ) {
            if( auto longValue = std::get_if< long >( &*token.value ) ) {
                return *longValue;
            }
        }

        Error{ error, token }.throwException();
        return 0;
    }

    static void expectTokenValue( const Token& token, const std::string& error ) {
        if( !token.value ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectTokenOfType( const Token& token, const TokenType& type, const std::string& error ) {
        if( token.type != type ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectTokenOfType( const Token& token, const std::set< TokenType >& acceptableTypes, const std::string& error ) {
        if( !acceptableTypes.count( token.type ) ) {
            Error{ error, token }.throwException();
        }
    }

    static void expectTokenType( const Token& token, const std::string& error ) {
        static const std::set< TokenType > VALID_TOKENS = {
            TokenType::TOKEN_U8,
            TokenType::TOKEN_U16,
            TokenType::TOKEN_U32,
            TokenType::TOKEN_S8,
            TokenType::TOKEN_S16,
            TokenType::TOKEN_S32,
            TokenType::TOKEN_STRING
        };

        if( ( token.type == TokenType::TOKEN_IDENTIFIER && ( !token.value || !std::holds_alternative< std::string >( *token.value ) ) ) ||
              !VALID_TOKENS.count( token.type ) ) {
            Error{ error, token }.throwException();
        }
    }

    static Token expectToken( const Primary& primary, std::optional< Token > nearestToken, const std::string& error ) {
        if( auto token = std::get_if< Token >( &primary.value ) ) {
            return *token;
        }

        Error{ error, nearestToken }.throwException();
        return Token{ TokenType::TOKEN_NONE, {}, 0, 0 };
    }

    // mostly checks for internal compiler errors
    static void check( const Primary& node, MemoryTracker& memory ) {
        std::visit( overloaded {

            []( const Token& token ) {
                switch( token.type ) {
                    case TokenType::TOKEN_IDENTIFIER: {
                        expectTokenValue( token, "Internal compiler error (Token of type TOKEN_IDENTIFIER has no associated value)" );
                        break;
                    }
                    case TokenType::TOKEN_LITERAL_STRING: {
                        expectTokenString( token, "Internal compiler error (Token of type TOKEN_LITERAL_STRING has no associated string)" );
                        break;
                    }
                    case TokenType::TOKEN_LITERAL_INTEGER: {
                        expectTokenLong( token, "Internal compiler error (Token of type TOKEN_LITERAL_INTEGER has no associated long)" );
                        break;
                    }
                    default:
                        // tests pass
                        break;
                }
            },

            [ &memory ]( const std::unique_ptr< Expression >& expression ) {
                check( *expression, memory );
            }

        }, node.value );
    }

    static void check( const BinaryExpression& node, std::optional< Token > nearestToken, MemoryTracker& memory ) {
        // Constraints on BinaryExpressions:
        // 1) Left-hand side expression and right-hand side expression must validate
        // 2) Operator must be token-type primary and one of the following: +, -, *, /, %, or .
        // 3) For . operator:
        // - Left-hand side must return a declared UDT type
        // - Right-hand side must be a token-type primary of identifier type
        // - Right-hand side identifier must be a valid field on the left-hand side user-defined type
        // 4) For all other operators:
        // - Left-hand side type must match right-hand side type, OR
        // - Right-hand side type must be coercible to left-hand side type:
        //  - Both are integer type, or
        //  - One side is a string while another is an integer type

        check( *node.lhsValue, memory );
        check( *node.op, memory );
        check( *node.rhsValue, memory );

        std::optional< std::string > lhsType = getType( *node.lhsValue, memory );
        std::optional< std::string > rhsType = getType( *node.rhsValue, memory );

        Token token = expectToken( *node.op, nearestToken, "Expected: Operator of BinaryExpression to be of Token type" );
        if( token.type == TokenType::TOKEN_DOT ) {
            // - Left-hand side must return a declared UDT type...
            if( !lhsType || !typeIsUdt( *lhsType ) ) {
                Error{ "Expected: Declared user-defined type as left-hand side of BinaryExpression with \".\" operator", token }.throwException();
            }

            if( !memory.findUdt( *lhsType ) ) {
                Error{ "Undeclared user-defined type: " + *lhsType, token }.throwException();
            }

            // - Right hand side must be a token-type primary....
            if( auto primaryType = std::get_if< std::unique_ptr< Primary > >( &node.rhsValue->value ) ) {
                // ...of identifier type
                Token rhsIdentifier = expectToken( **primaryType, nearestToken, "Expected: Expression of Primary token type as right-hand side of BinaryExpression with \".\" operator" );
                expectTokenOfType( rhsIdentifier, TokenType::TOKEN_IDENTIFIER, "Primary expression in RHS of BinaryExpression with \".\' operator must be of identifier type" );

                std::string rhsUdtFieldId = expectTokenString( rhsIdentifier, "Internal compiler error (BinaryExpression dot RHS token has no string alternative)" );
                if( !memory.findUdtField( *lhsType, rhsUdtFieldId ) ) {
                    Error{ "Invalid field " + rhsUdtFieldId + " on user-defined type " + *lhsType, rhsIdentifier }.throwException();
                }
            } else {
                Error{ "Expected: Expression of Primary type as right-hand side of BinaryExpression with \".\" operator", nearestToken }.throwException();
            }
        }

        expectTokenOfType(
            token,
            {
                TokenType::TOKEN_PLUS,
                TokenType::TOKEN_MINUS,
                TokenType::TOKEN_ASTERISK,
                TokenType::TOKEN_FORWARD_SLASH,
                TokenType::TOKEN_MODULO
            },
            "Expected: Operator of BinaryExpression to be one of \"+\",\"-\",\"*\",\"/\",\"%\",\".\""
        );

        // Check if types are identical, and if not identical, if they can be coerced
        if( !( typesMatch( *lhsType, *rhsType ) || integerTypesMatch( *lhsType, *rhsType ) || coercibleToString( *lhsType, *rhsType ) ) ) {
            Error{ "Type mismatch: Expected type " + *lhsType + " but right-hand side expression is of type " + *rhsType, nearestToken }.throwException();
        }
    }

    static void check( const AssignmentExpression& node, std::optional< Token > nearestToken, MemoryTracker& memory ) {
        // Begin with a simple verification of both the left-hand side and the right-hand side
        check( *node.identifier, memory );
        check( *node.expression, memory );

        // Left hand side must be either primary expression type with identifier, or binaryexpression type with dot operator
        const Expression& identifierExpression = *node.identifier;
        if( auto primaryExpression = std::get_if< std::unique_ptr< Primary > >( &identifierExpression.value ) ) {
            const Primary& primary = **primaryExpression;

            Token token = expectToken( primary, identifierExpression.nearestToken, "Expected: Primary expression in LHS of AssignmentExpression must be a single identifier" );

            // Primary expression must contain a token of type IDENTIFIER
            expectTokenOfType( token, TokenType::TOKEN_IDENTIFIER, "Primary expression in LHS of AssignmentExpression must be a single identifier" );
            expectTokenString( token, "Internal compiler error (AssignmentExpression token has no string alternative" );
        } else if( auto result = std::get_if< std::unique_ptr< BinaryExpression > >( &identifierExpression.value ) ) {
            // Validate this binary expression
            const BinaryExpression& binaryExpression = **result;

            // The above binary expression may validate as correct, but in this case, it must be a dot expression
            Token token = expectToken( *binaryExpression.op, identifierExpression.nearestToken, "BinaryExpression must have an operator of Token type" );
            expectTokenOfType( token, TokenType::TOKEN_DOT, "BinaryExpression must have operator \".\" for left-hand side of AssignmentExpression" );
        } else {
            Error{ "Invalid left-hand expression type for AssignmentExpression", nearestToken }.throwException();
        }

        // Type of right hand side assignment should match type of identifier on left hand side
        std::optional< std::string > lhsType = getType( *node.identifier, memory );
        if( !lhsType ) {
            Error{ "Internal compiler error (AssignmentExpression unable to determine type for node.identifier)", nearestToken }.throwException();
        }

        std::optional< std::string > rhsType = getType( *node.expression, memory );
        if( !rhsType ) {
            Error{ "Internal compiler error (AssignmentExpression unable to determine type for node.expression", nearestToken }.throwException();
        }

        if( !( typesMatch( *lhsType, *rhsType ) || integerTypesMatch( *lhsType, *rhsType ) ) ) {
            Error{ "Type mismatch: Expected type " + *lhsType + " but expression is of type " + *rhsType, nearestToken }.throwException();
        }
    }

    static void check( const Expression& node, MemoryTracker& memory ) {
        std::visit( overloaded {

            [ &node, &memory ]( const std::unique_ptr< AssignmentExpression >& expression ) { check( *expression, node.nearestToken, memory ); },
            [ &node, &memory ]( const std::unique_ptr< BinaryExpression >& expression ) { check( *expression, node.nearestToken, memory ); },
            []( const std::unique_ptr< UnaryExpression >& expression ) { Error{ "Internal compiler error (Expression check not implemented for expression subtype UnaryExpression)", {} }.throwException(); },
            []( const std::unique_ptr< CallExpression >& expression ) { Error{ "Internal compiler error (Expression check not implemented for expression subtype CallExpression)", {} }.throwException(); },
            [ &memory ]( const std::unique_ptr< Primary >& expression ) { check( *expression, memory ); },

        }, node.value );
    }

    static void check( const VarDeclaration& node, MemoryTracker& memory ) {
        // def x as type = value

        // Verify x is an identifier containing a string
        expectTokenOfType( node.variable.name, TokenType::TOKEN_IDENTIFIER, "Expected: Identifier as token type for parameter declaration" );
        expectTokenString( node.variable.name, "Internal compiler error (VarDeclaration variable.name has no string alternative)" );

        // Verify type is either primitive or declared
        expectTokenType( node.variable.type.type, "Expected: Declared user-defined type or one of [u8, u16, u32, s8, s16, s32, string]" );

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

    static void check( const Statement& node, MemoryTracker& memory ) {
        std::visit( overloaded {

            [ &memory ]( const std::unique_ptr< ExpressionStatement >& statement ) { check( *(statement->value), memory ); },
			[]( const std::unique_ptr< ForStatement >& statement ) { Error{ "Internal compiler error (Statement check not implemented for statement subtype ForStatement)", {} }.throwException(); },
			[]( const std::unique_ptr< IfStatement >& statement ) { Error{ "Internal compiler error (Statement check not implemented for statement subtype IfStatement)", {} }.throwException(); },
			[]( const std::unique_ptr< ReturnStatement >& statement ) { Error{ "Internal compiler error (Statement check not implemented for statement subtype ReturnStatement)", {} }.throwException(); },
			[]( const std::unique_ptr< AsmStatement >& statement ) { Error{ "Internal compiler error (Statement check not implemented for statement subtype AsmStatement)", {} }.throwException(); },
			[]( const std::unique_ptr< WhileStatement >& statement ) { Error{ "Internal compiler error (Statement check not implemented for statement subtype WhileStatement)", {} }.throwException(); }

        }, node.value );
    }

    static void check( const Declaration& node, MemoryTracker& memory ) {
        std::visit( overloaded {

            []( const std::unique_ptr< Annotation >& declaration ) { Error{ "Internal compiler error (Declaration check not implemented for declaration subtype Annotation)", {} }.throwException(); },
            [ &memory ]( const std::unique_ptr< VarDeclaration >& declaration ) { check( *declaration, memory ); },
            []( const std::unique_ptr< ConstDeclaration >& declaration ) { Error{ "Internal compiler error (Declaration check not implemented for declaration subtype ConstDeclaration)", {} }.throwException(); },
            []( const std::unique_ptr< FunctionDeclaration >& declaration ) { Error{ "Internal compiler error (Declaration check not implemented for declaration subtype FunctionDeclaration)", {} }.throwException(); },
            []( const std::unique_ptr< TypeDeclaration >& declaration ) { Error{ "Internal compiler error (Declaration check not implemented for declaration subtype TypeDeclaration)", {} }.throwException(); },
            []( const std::unique_ptr< ImportDeclaration >& declaration ) { Error{ "Internal compiler error (Declaration check not implemented for declaration subtype ImportDeclaration)", {} }.throwException(); },
            [ &memory ]( const std::unique_ptr< Statement >& declaration ) { check( *declaration, memory ); }

        }, node.value );
    }

    /**
     * Run a verification step to make sure items are logically consistent
     */
    std::optional< std::string > check( const Program& program ) {
        MemoryTracker memory;

        for( const auto& declaration : program.statements ) {
            try {
                check( *declaration, memory );
            } catch( std::runtime_error e ) {
                return e.what();
            }
        }

        return {};
    }

}