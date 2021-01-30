#include "tree_tools.hpp"
#include "error.hpp"
#include <variant>

namespace GoldScorpion {

    static void evaluateConstantExpression( const Expression& node, ConstEvaluationSettings settings );

    bool constantIsArray( const ConstantExpressionValue& value ) {
        return std::holds_alternative< std::vector< long > >( value ) ||
               std::holds_alternative< std::vector< std::string > >( value );
    }

    std::optional< std::string > getIdentifierName( const Token& token ) {
        if( token.type == TokenType::TOKEN_IDENTIFIER && token.value ) {
            if( auto stringResult = std::get_if< std::string >( &*token.value ) ) {
                return *stringResult;
            }
        }

        return {};
    }

	std::optional< std::string > getIdentifierName( const Expression& node ) {
        if( auto primaryResult = std::get_if< std::unique_ptr< Primary > >( &node.value ) ) {
            const Primary& primary = **primaryResult;
            if( auto tokenResult = std::get_if< Token >( &primary.value ) ) {
                const Token& token = *tokenResult;
                return getIdentifierName( token );
            }
        }

        return {};
	}

    bool containsReturn( const WhileStatement& node ) {
        for( const auto& declaration : node.body ) {
            if( auto result = std::get_if< std::unique_ptr< Statement > >( &declaration->value ) ) {
                const std::unique_ptr< Statement >& statement = *result;

                if( std::holds_alternative< std::unique_ptr< ReturnStatement > >( statement->value ) ) {
                    return true;
                }

                if( auto result = std::get_if< std::unique_ptr< ForStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< IfStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< WhileStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    // The IfStatement must return in any condition for this to return true. This means:
    // - There must be an "else" condition
    // - There must be a return statement in all conditions
    bool containsReturn( const IfStatement& node ) {
        // The IfStatement cannot guarantee a return if there are not more bodies than conditions
        // AKA there is no "else" statement
        if( node.bodies.size() <= node.conditions.size() ) {
            return false;
        }

        bool result = false;
        for( const auto& conditionBody : node.bodies ) {
            bool conditionBodyResult = false;

            for( const auto& declaration : conditionBody ) {
                if( auto result = std::get_if< std::unique_ptr< Statement > >( &declaration->value ) ) {
                    const std::unique_ptr< Statement >& statement = *result;

                    if( std::holds_alternative< std::unique_ptr< ReturnStatement > >( statement->value ) ) {
                        conditionBodyResult = true; break;
                    }

                    if( auto result = std::get_if< std::unique_ptr< ForStatement > >( &statement->value ) ) {
                        if( containsReturn( **result ) ) {
                            conditionBodyResult = true; break;
                        }
                    }

                    if( auto result = std::get_if< std::unique_ptr< IfStatement > >( &statement->value ) ) {
                        if( containsReturn( **result ) ) {
                            conditionBodyResult = true; break;
                        }
                    }

                    if( auto result = std::get_if< std::unique_ptr< WhileStatement > >( &statement->value ) ) {
                        if( containsReturn( **result ) ) {
                            conditionBodyResult = true; break;
                        }
                    }
                }
            }

            // If it returns false, don't waste any more time
            if( !conditionBodyResult ) {
                return false;
            } else {
                result = true;
            }
        }

        return result;
    }

    bool containsReturn( const ForStatement& node ) {
        for( const auto& declaration : node.body ) {
            if( auto result = std::get_if< std::unique_ptr< Statement > >( &declaration->value ) ) {
                const std::unique_ptr< Statement >& statement = *result;

                if( std::holds_alternative< std::unique_ptr< ReturnStatement > >( statement->value ) ) {
                    return true;
                }

                if( auto result = std::get_if< std::unique_ptr< ForStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< IfStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< WhileStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    /**
     * Search for return statements in a function body, in the following method:
     * - Return statement occurs directly in the function body
     * - Return statement occurs anywhere in *all* conditions of an IfStatement
     * - Return statement occurs anywhere in a ForLoop
     * - Return statement occurs anywhere in a WhileLoop
     */
    bool containsReturn( const FunctionDeclaration& node ) {
        for( const auto& declaration : node.body ) {
            if( auto result = std::get_if< std::unique_ptr< Statement > >( &declaration->value ) ) {
                const std::unique_ptr< Statement >& statement = *result;

                // Return statement was encountered directly on the function body
                if( std::holds_alternative< std::unique_ptr< ReturnStatement > >( statement->value ) ) {
                    return true;
                }

                // Check inside all conditions of one of the following statements
                if( auto result = std::get_if< std::unique_ptr< ForStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< IfStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }

                if( auto result = std::get_if< std::unique_ptr< WhileStatement > >( &statement->value ) ) {
                    if( containsReturn( **result ) ) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    static void evaluateConstantExpression( const Primary& node, ConstEvaluationSettings settings ) {
        // The only acceptable types here are subexpressions, or tokens of either literal integer, literal string, or identifier type
        if( auto subexpression = std::get_if< std::unique_ptr< Expression > > ( &node.value ) ) {
            return evaluateConstantExpression( **subexpression, settings );
        }

        Token token = std::get< Token >( node.value );
        switch( token.type ) {
            case TokenType::TOKEN_LITERAL_INTEGER: {
                settings.stack.push( std::get< long >( *( token.value ) ) );
                return;
            }
            case TokenType::TOKEN_LITERAL_STRING: {
                settings.stack.push( std::get< std::string >( *( token.value ) ) );
                return;
            }
            case TokenType::TOKEN_IDENTIFIER: {
                // Get identifier, then get type. Must return a constant symbol.
                std::string identifier = std::get< std::string >( *( token.value ) );
                auto symbolQuery = settings.symbols.findSymbol( settings.fileId, identifier );
                if( !symbolQuery ) {
                    Error{ "Cannot find symbol: " + identifier, token }.throwException();
                }

                if( !std::holds_alternative< ConstantSymbol >( symbolQuery->symbol ) ) {
                    Error{ "Symbol \"" + std::get< std::string >( *( token.value ) ) + "\" is a non-constant symbol", token }.throwException();
                }

                settings.stack.push( std::get< ConstantSymbol >( symbolQuery->symbol ).value );
                return;
            }
            default:
                Error{ "Internal compiler error (Token of unexpected type encountered while trying to evaluate constant expression", token }.throwException();
        }
    }

    static void evaluateConstantExpression( const BinaryExpression& node, ConstEvaluationSettings settings ) {
        // Push both sides onto the stack - beginning with the right
        evaluateConstantExpression( *node.rhsValue, settings );
        evaluateConstantExpression( *node.lhsValue, settings );

        ConstantExpressionValue left = settings.stack.top();
        settings.stack.pop();
        ConstantExpressionValue right = settings.stack.top();
        settings.stack.pop();

        // Attempt to extract operator
        Token operatorToken;
        if( auto token = std::get_if< Token >( &node.op->value ) ) {
            operatorToken = *token;
        } else {
            Error{ "Internal compiler error (BinaryExpression operator not of token type)", settings.nearestToken }.throwException();
        }


        if( constantIsArray( left ) || constantIsArray( right ) ) {
            // Cannot apply a binaryexpression operation to an array
            Error{ "Array type invalid as operand in constant BinaryExpression", operatorToken }.throwException();
        } else if( std::holds_alternative< std::string >( left ) || std::holds_alternative< std::string >( right ) ) {
            // If either side contains a string then the total value will be coerced to string, and the "+" operator is the only valid operator.
            if( operatorToken.type != TokenType::TOKEN_PLUS ) {
                Error{ "Only the concatenation \"+\" operator is valid for an expression combining a numeric and a string type", operatorToken }.throwException();
            }

            std::string result;
            if( auto leftString = std::get_if< std::string >( &left ) ) {
                result = *leftString;
                result += std::to_string( std::get< long >( right ) );
            } else {
                result = std::to_string( std::get< long >( left ) );
                result += std::get< std::string >( right );
            }

            settings.stack.push( result );
        } else {
            // Both sides are long and can be operated on directly
            switch( operatorToken.type ) {
                case TokenType::TOKEN_PLUS: {
                    settings.stack.push( std::get< long >( left ) + std::get< long >( right ) );
                    return;
                }
                case TokenType::TOKEN_MINUS: {
                    settings.stack.push( std::get< long >( left ) - std::get< long >( right ) );
                    return;
                }
                case TokenType::TOKEN_ASTERISK: {
                    settings.stack.push( std::get< long >( left ) * std::get< long >( right ) );
                    return;
                }
                case TokenType::TOKEN_FORWARD_SLASH: {
                    settings.stack.push( std::get< long >( left ) / std::get< long >( right ) );
                    return;
                }
                default:
                    Error{ "Invalid operator in constant expression", operatorToken }.throwException();
            }
        }
    }

    static void evaluateConstantExpression( const UnaryExpression& node, ConstEvaluationSettings settings ) {
        evaluateConstantExpression( *node.value, settings );

        ConstantExpressionValue operand = settings.stack.top();
        settings.stack.pop();

        Token operatorToken;
        if( auto token = std::get_if< Token >( &node.op->value ) ) {
            operatorToken = *token;
        } else {
            Error{ "Internal compiler error (UnaryExpression operator not of token type)", settings.nearestToken }.throwException();
        }

        if( constantIsArray( operand ) ) {
            // Cannot apply a unaryexpression operation to an array
            Error{ "Array type invalid as operand in constant UnaryExpression", operatorToken }.throwException();
        }

        // Strings not valid for unary expression
        if( std::holds_alternative< std::string >( operand ) ) {
            Error{ "String operand not valid for unary expression in constant", operatorToken }.throwException();
        }

        switch( operatorToken.type ) {
            case TokenType::TOKEN_MINUS: {
                settings.stack.push( std::get< long >( operand ) * -1 );
                return;
            }
            case TokenType::TOKEN_NOT: {
                settings.stack.push( !std::get< long >( operand ) );
                return;
            }
            default:
                Error{ "Invalid operator in constant expression", operatorToken }.throwException();
        }
    }

    static void evaluateConstantExpression( const Expression& node, ConstEvaluationSettings settings ) {
        if( auto primary = std::get_if< std::unique_ptr< Primary > >( &node.value ) ) {
            return evaluateConstantExpression( **primary, settings );
        }

        if( auto binary = std::get_if< std::unique_ptr< BinaryExpression > >( &node.value ) ) {
            return evaluateConstantExpression( **binary, settings );
        }

        if( auto unary = std::get_if< std::unique_ptr< UnaryExpression > >( &node.value ) ) {
            return evaluateConstantExpression( **unary, settings );
        }

        Error{ "Expression contains non-constant part and cannot be evaluated at compile-time", settings.nearestToken }.throwException();
    }

    ConstantExpressionValue evaluateConst( const Expression& node, ConstEvaluationSettings settings ) {
        evaluateConstantExpression( node, settings );
        return settings.stack.top();
    }

}