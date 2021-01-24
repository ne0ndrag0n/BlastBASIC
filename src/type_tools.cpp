#include "type_tools.hpp"
#include "tree_tools.hpp"
#include "variant_visitor.hpp"
#include "error.hpp"
#include "utility.hpp"

namespace GoldScorpion {

    static std::string getLiteralType( long literal ) {
		// Negative values mean a signed value is required
		if( literal < 0 ) {

			if( literal >= -127 ) {
				return "s8";
			}

			if( literal >= -32767 ) {
				return "s16";
			}

			return "s32";

		} else {

			if( literal <= 255 ) {
				return "u8";
			}

			if( literal <= 65535 ) {
				return "u16";
			}

			return "u32";
		}
	}

	static long expectLong( const Token& token ) {
		if( token.value ) {
			if( auto longValue = std::get_if< long >( &*( token.value ) ) ) {
				return *longValue;
			}
		}

		Error{ "Internal compiler error", token }.throwException();
		return 0;
	}

	static std::string expectString( const Token& token ) {
		if( token.value ) {
			if( auto stringValue = std::get_if< std::string >( &*( token.value ) ) ) {
				return *stringValue;
			}
		}

		Error{ "Internal compiler error", token }.throwException();
		return "";
	}

    static char getTypeComparison( const SymbolNativeType& symbolType ) {
        if( symbolType.type == TokenType::TOKEN_U8 || symbolType.type == TokenType::TOKEN_S8 ) {
            return 0;
        }

        if( symbolType.type == TokenType::TOKEN_U16 || symbolType.type == TokenType::TOKEN_S16 ) {
            return 1;
        }

        if( symbolType.type == TokenType::TOKEN_U32 || symbolType.type == TokenType::TOKEN_S32 || symbolType.type == TokenType::TOKEN_STRING ) {
            return 2;
        }

        return 3;
    }

    static bool isSigned( const SymbolNativeType& symbolType ) {
        return symbolType.type == TokenType::TOKEN_S8 || symbolType.type == TokenType::TOKEN_S16 || symbolType.type == TokenType::TOKEN_S32;
    }

    static bool isOneSigned( const SymbolNativeType& a, const SymbolNativeType& b ) {
        return ( isSigned( a ) && !isSigned( b ) ) || ( !isSigned( a ) && isSigned( b ) );
    }

    static SymbolNativeType scrubSigned( const SymbolNativeType& symbolType ) {
        if( symbolType.type == TokenType::TOKEN_S8 ) { return SymbolNativeType{ TokenType::TOKEN_U8 }; }
        if( symbolType.type == TokenType::TOKEN_S16 ) { return SymbolNativeType{ TokenType::TOKEN_U16 }; }
        if( symbolType.type == TokenType::TOKEN_S32 ) { return SymbolNativeType{ TokenType::TOKEN_U32 }; }

        return symbolType;
    }

    std::optional< TokenType > typeIdToTokenType( const std::string& id ) {
        if( id == "u8" ) { return TokenType::TOKEN_U8; }
        if( id == "u16" ) { return TokenType::TOKEN_U16; }
        if( id == "u32" ) { return TokenType::TOKEN_U32; }
        if( id == "s8" ) { return TokenType::TOKEN_S8; }
        if( id == "s16" ) { return TokenType::TOKEN_S16; }
        if( id == "s32" ) { return TokenType::TOKEN_S32; }
        if( id == "string" ) { return TokenType::TOKEN_STRING; }

        return {};
    }

    std::optional< std::string > tokenTypeToTypeId( const TokenType type ) {
        switch( type ) {
            default:
                return {};
            case TokenType::TOKEN_U8:
                return "u8";
            case TokenType::TOKEN_U16:
                return "u16";
            case TokenType::TOKEN_U32:
                return "u32";
            case TokenType::TOKEN_S8:
                return "s8";
            case TokenType::TOKEN_S16:
                return "s16";
            case TokenType::TOKEN_S32:
                return "s32";
            case TokenType::TOKEN_STRING:
                return "string";
        }
    }

    std::optional< std::string > tokenToTypeId( const Token& token ) {
        // Attempt to pull a type from the TokenType
        std::optional< std::string > typeId = tokenTypeToTypeId( token.type );
        if( typeId ) {
            return typeId;
        }

        // If that wasn't possible, then let's try to extract from an identifier w/string
        if( token.type == TokenType::TOKEN_IDENTIFIER && token.value ) {
            if( auto stringValue = std::get_if< std::string >( &*token.value ) ) {
                return *stringValue;
            }
        }

        return {};
    }

    bool tokenIsPrimitiveType( const Token& token ) {
        switch( token.type ) {
            default:
                return false;
            case TokenType::TOKEN_U8:
            case TokenType::TOKEN_U16:
            case TokenType::TOKEN_U32:
            case TokenType::TOKEN_S8:
            case TokenType::TOKEN_S16:
            case TokenType::TOKEN_S32:
            case TokenType::TOKEN_STRING:
                return true;
        }
    }

    bool typesComparable( const SymbolType& lhs, const SymbolType& rhs ) {
        // No symbol type is comparable to another symbol type (the two must be the same variant)
        return lhs.index() == rhs.index();
    }

    bool typeIsFunction( const SymbolType& type ) {
        return std::holds_alternative< SymbolFunctionType >( type );
    }

    bool typeIsUdt( const SymbolType& type ) {
        return std::holds_alternative< SymbolUdtType >( type );
    }

    bool typeIsInteger( const SymbolType& type ) {
        if( auto nativeType = std::get_if< SymbolNativeType >( &type ) ) {
            return
                nativeType->type == TokenType::TOKEN_U8 ||
                nativeType->type == TokenType::TOKEN_U16 ||
                nativeType->type == TokenType::TOKEN_U32 ||
                nativeType->type == TokenType::TOKEN_S8 ||
                nativeType->type == TokenType::TOKEN_S16 ||
                nativeType->type == TokenType::TOKEN_S32;
        }

        return false;
    }

    bool typeIsString( const SymbolType& type ) {
        if( auto nativeType = std::get_if< SymbolNativeType >( &type ) ) {
            return nativeType->type == TokenType::TOKEN_STRING;
        }

        return false;
    }

    // This function does not work if functions can take other functions as arguments, or return other functions as return types
    bool typesMatch( const SymbolType& lhs, const SymbolType& rhs, SymbolTypeSettings settings ) {
        if( !typesComparable( lhs, rhs ) ) {
            return false;
        }

        if( typeIsFunction( lhs ) ) {
            // Functions are only the same type if they contain the same arguments + return type
            auto lhsFunctionQuery = settings.symbols.findSymbol( settings.fileId, getSymbolTypeId( lhs ) );
            if( !lhsFunctionQuery || !std::holds_alternative< FunctionSymbol >( lhsFunctionQuery->symbol ) ) {
                Error{ "Internal compiler error (unable to find symbol that is claimed to exist)", {} }.throwException();
            }

            auto rhsFunctionQuery = settings.symbols.findSymbol( settings.fileId, getSymbolTypeId( rhs ) );
            if( !rhsFunctionQuery || !std::holds_alternative< FunctionSymbol >( rhsFunctionQuery->symbol ) ) {
                Error{ "Internal compiler error (unable to find symbol that is claimed to exist)", {} }.throwException();
            }

            const FunctionSymbol& lhsFunction = std::get< FunctionSymbol >( lhsFunctionQuery->symbol );
            const FunctionSymbol& rhsFunction = std::get< FunctionSymbol >( rhsFunctionQuery->symbol );

            // First the easy checks - Must have same function return type and number of function arguments
            if( lhsFunction.functionReturnType.has_value() != rhsFunction.functionReturnType.has_value() ) {
                return false;
            }

            if( lhsFunction.functionReturnType && ( getSymbolTypeId( *lhsFunction.functionReturnType ) != getSymbolTypeId( *rhsFunction.functionReturnType ) ) ) {
                return false;
            }

            if( lhsFunction.arguments.size() != rhsFunction.arguments.size() ) {
                return false;
            }

            // Now we need to check each SymbolArgument
            for( size_t i = 0; i != lhsFunction.arguments.size(); i++ ) {
                if( ( lhsFunction.arguments[ i ].id != rhsFunction.arguments[ i ].id ) ||
                    ( getSymbolTypeId( lhsFunction.arguments[ i ].type ) != getSymbolTypeId( rhsFunction.arguments[ i ].type ) ) ) {
                    return false;
                }
            }

            return true;
        } else {
            return getSymbolTypeId( lhs ) == getSymbolTypeId( rhs );
        }
    }

    bool integerTypesMatch( const SymbolType& lhs, const SymbolType& rhs ) {
        return typeIsInteger( lhs ) && typeIsInteger( rhs );
    }

    bool assignmentCoercible( const SymbolType& lhs, const SymbolType& rhs ) {
        if( auto lhsNative = std::get_if< SymbolNativeType >( &lhs ) ) {
            return lhsNative->type == TokenType::TOKEN_STRING && typeIsInteger( rhs );
        }

        return false;
    }

    bool coercibleToString( const SymbolType& lhs, const SymbolType& rhs ) {
        // Functions and UDTs are not coercible to strings
        // Both must be a SymbolNativeType; one must be string and one must be nonstring
        if( auto lhsNativeType = std::get_if< SymbolNativeType >( &lhs ) ) {
            if( auto rhsNativeType = std::get_if< SymbolNativeType >( &rhs ) ) {
                return ( lhsNativeType->type == TokenType::TOKEN_STRING || rhsNativeType->type == TokenType::TOKEN_STRING ) &&
                       ( typeIsInteger( lhs ) || typeIsInteger( rhs ) );
            }
        }

        return false;
    }

    SymbolNativeType promotePrimitiveTypes( const SymbolNativeType& lhs, const SymbolNativeType& rhs ) {
        if( getTypeComparison( rhs ) >= getTypeComparison( lhs ) ) {
            return SymbolNativeType{ isOneSigned( lhs, rhs ) ? scrubSigned( rhs ) : rhs };
        } else {
            return SymbolNativeType{ isOneSigned( lhs, rhs ) ? scrubSigned( lhs ) : lhs };
        }
    }

    // This is the new shit
    SymbolTypeResult getType( const Primary& node, SymbolTypeSettings settings ) {

        if( auto expressionSubtype = std::get_if< std::unique_ptr< Expression > >( &node.value ) ) {
            return getType( **expressionSubtype, settings );
        }

        const Token& token = std::get< Token >( node.value );
        switch( token.type ) {
            case TokenType::TOKEN_LITERAL_INTEGER: {
                return SymbolTypeResult::good( SymbolNativeType{ *typeIdToTokenType( getLiteralType( expectLong( token ) ) ) } );
            }
            case TokenType::TOKEN_LITERAL_STRING: {
                return SymbolTypeResult::good( SymbolNativeType{ TokenType::TOKEN_STRING } );
            }
            case TokenType::TOKEN_THIS: {
                // Type of "this" token is obtainable from the pointer on the stack
                auto thisQuery = settings.symbols.findSymbol( settings.fileId, "this" );
                if( !thisQuery ) {
                    Error{ "Internal compiler error (unable to determine type of \"this\" token)", token }.throwException();
                }

                // Symbol type of thisQuery must be VariableSymbol
                if( auto variableSymbol = std::get_if< VariableSymbol >( &( thisQuery->symbol ) ) ) {
                    if( auto udtType = std::get_if< SymbolUdtType >( &( variableSymbol->type ) ) ) {
                        return SymbolTypeResult::good( *udtType );
                    } else {
                        Error{ "Internal compiler error (\"this\" token only valid for SymbolUdtType)", token }.throwException();
                    }
                } else {
                    Error{ "Internal compiler error (type of \"this\" token does not point to instance of a variable)", token }.throwException();
                }

                return SymbolTypeResult::err( "Internal compiler error" );
            }
            case TokenType::TOKEN_IDENTIFIER: {
                // Look up identifier in memory
                std::string id = expectString( token );
                auto memoryQuery = settings.symbols.findSymbol( settings.fileId, id );
                if( !memoryQuery ) {
                    return SymbolTypeResult::err( "Undefined symbol: " + id );
                }

                // Determine whether to return FunctionType or ValueType by identifier returned
                if( auto variableSymbol = std::get_if< VariableSymbol >( &( memoryQuery->symbol ) ) ) {
                    return SymbolTypeResult::good( variableSymbol->type );
                } else if( auto constantSymbol = std::get_if< ConstantSymbol >( &( memoryQuery->symbol ) ) ) {
                    return SymbolTypeResult::good( constantSymbol->type );
                } else if( auto functionSymbol = std::get_if< FunctionSymbol >( &( memoryQuery->symbol ) ) ) {
                    return SymbolTypeResult::good( SymbolFunctionType{ functionSymbol->id } );
                } else {
                    Error{ "Internal compiler error (Identifier token cannot indirectly refer to UDT)", token }.throwException();
                }

                return SymbolTypeResult::err( "Internal compiler error" );
            }
            default: {
                Error{ "Internal compiler error (Token not of expected type for SymbolTypeResult)", token }.throwException();
                return SymbolTypeResult::err( "Internal compiler error" );
            }

        }

    }

    SymbolTypeResult getType( const CallExpression& node, SymbolTypeSettings settings ) {
        // Get the return type of the expression
        SymbolTypeResult expressionType = getType( *node.identifier, settings );
        if( !expressionType ) {
            return SymbolTypeResult::err( "Could not deduce type of identifier in CallExpression: " + expressionType.getError() );
        }

        // The expression must ultimately boil down to a function
        if( !typeIsFunction( *expressionType ) ) {
            return SymbolTypeResult::err( "Unable to call non-function type " + getSymbolTypeId( *expressionType ) );
        }

        // The return type of the function is the type of this CallExpression
        const SymbolFunctionType& functionRef = std::get< SymbolFunctionType >( *expressionType );
        auto functionQuery = settings.symbols.findSymbol( settings.fileId, functionRef.id );
        if( !functionQuery || !std::holds_alternative< FunctionSymbol >( functionQuery->symbol ) ) {
            Error{ "Internal compiler error (function symbol said to exist does not exist)", {} }.throwException();
        }

        const FunctionSymbol& function = std::get< FunctionSymbol >( functionQuery->symbol );
        if( !function.functionReturnType ) {
            return SymbolTypeResult::err( "Cannot call function with no return type" );
        }

        return SymbolTypeResult::good( *function.functionReturnType );
    }

    SymbolTypeResult getType( const BinaryExpression& node, SymbolTypeSettings settings ) {

        SymbolTypeResult lhs = getType( *node.lhsValue, settings );
        if( !lhs ) {
            return lhs;
        }
        SymbolTypeResult rhs = getType( *node.rhsValue, settings );

        // If operator is dot then type of the expression is the field on the left hand side
        TokenType tokenOp;
        if( auto token = std::get_if< Token >( &node.op->value ) ) {
            tokenOp = token->type;
        } else {
            Error{ "Internal compiler error (BinaryExpression op does not contain Token variant)", {} }.throwException();
        }

        switch( tokenOp ) {
            case TokenType::TOKEN_DOT: {
                // The type of a dot operation is the field, specified on the RHS, of the UDT on the LHS.
                auto rhsIdentifier = getIdentifierName( *node.rhsValue );
                if( !rhsIdentifier ) {
                    return SymbolTypeResult::err( "Right-hand side of dot operator must contain single identifier" );
                }

                std::string typeId = getSymbolTypeId( *lhs );
                auto lhsUdt = settings.symbols.findSymbol( settings.fileId, typeId );
                if( !lhsUdt ) {
                    return SymbolTypeResult::err( "Undeclared user-defined type" );
                }

                if( auto asUdt = std::get_if< UdtSymbol >( &( lhsUdt->symbol ) ) ) {
                    // Find field of name rhsIdentifier
                    for( const SymbolField& argument : asUdt->fields ) {
                        if( argument.id == *rhsIdentifier ) {
                            // Field name found
                            // Discriminate VariableSymbol or FunctionSymbol and return that
                            if( auto asVariable = std::get_if< VariableSymbol >( &argument.value ) ) {
                                return SymbolTypeResult::good( ( *asVariable ).type );
                            }

                            if( std::holds_alternative< FunctionSymbol >( argument.value ) ) {
                                return SymbolTypeResult::good( SymbolFunctionType{ argument.id } );
                            }

                            return SymbolTypeResult::err( "Internal compiler error (unexpected UDT field type)" );
                        }
                    }

                    // If we got here, field name wasn't found
                    return SymbolTypeResult::err( "User-defined type " + typeId + " does not have field of name " + *rhsIdentifier );
                } else {
                    return SymbolTypeResult::err( "Cannot apply dot operator to non-user-defined type " + typeId );
                }
            }
            default: {
                // All other operators require both sides to have a well-defined type
                if( !rhs ) { return rhs; }

                // For all other operators, if left hand side is a UDT, then RHS must be as well.
                if( typeIsUdt( *lhs ) || typeIsFunction( *lhs ) ) {
                    if( typesMatch( *lhs, *rhs, settings ) ) {
                        return lhs;
                    } else {
                        return SymbolTypeResult::err( "Type mismatch" );
                    }
                }

                // If we get here, LHS is a SymbolNativeType. RHS must be SymbolNativeType as well.
                if( auto rhsNativeType = std::get_if< SymbolNativeType >( &*rhs ) ) {
                    return SymbolTypeResult::good(
                        promotePrimitiveTypes( std::get< SymbolNativeType >( *lhs ), *rhsNativeType )
                    );
                } else {
                    return SymbolTypeResult::err( "Type mismatch" );
                }
            }
        }
    }

    SymbolTypeResult getType( const AssignmentExpression& node, SymbolTypeSettings settings ) {
        // An assignment expression returns the type of the LHS ***IFF*** the RHS matches
        SymbolTypeResult lhs = getType( *node.identifier, settings );
        if( !lhs ) {
            return SymbolTypeResult::err( "Unable to obtain type of left-hand side of AssignmentExpression: " + lhs.getError() );
        }

        SymbolTypeResult rhs = getType( *node.expression, settings );
        if( !rhs ) {
            return SymbolTypeResult::err( "Unable to obtain type of right-hand side of AssignmentExpression: " + rhs.getError() );
        }

        if( typesMatch( *lhs, *rhs, settings ) || integerTypesMatch( *lhs, *rhs ) || assignmentCoercible( *lhs, *rhs ) ) {
            return SymbolTypeResult::good( *lhs );
        } else {
            return SymbolTypeResult::err( "Types in AssignmentExpression mismatch: " + getSymbolTypeId( *lhs ) + ", " + getSymbolTypeId( *rhs ) );
        }
    }

    SymbolTypeResult getType( const Expression& node, SymbolTypeSettings settings ) {
		if( auto binaryExpression = std::get_if< std::unique_ptr< BinaryExpression > >( &node.value ) ) {
 			return getType( **binaryExpression, settings );
		}

		if( auto primaryExpression = std::get_if< std::unique_ptr< Primary > >( &node.value ) ) {
			return getType( **primaryExpression, settings );
		}

        if( auto callExpression = std::get_if< std::unique_ptr< CallExpression > >( &node.value ) ) {
            return getType( **callExpression, settings );
        }

        if( auto unaryExpression = std::get_if< std::unique_ptr< UnaryExpression > >( &node.value ) ) {
            // Operators in a unary expression do not change their type
            return getType( *( ( **unaryExpression ).value ), settings );
        }

        if( auto assignmentExpression = std::get_if< std::unique_ptr< AssignmentExpression > >( &node.value ) ) {
            return getType( **assignmentExpression, settings );
        }

        return SymbolTypeResult::err( "Expression subtype not implemented" );
    }

}
