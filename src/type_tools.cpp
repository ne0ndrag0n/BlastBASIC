#include "type_tools.hpp"
#include "variant_visitor.hpp"
#include "error.hpp"

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

    static char getTypeComparison( const std::string& id ) {
        if( id == "u8" || id == "s8" ) {
            return 0;
        }

        if( id == "u16" || id == "s16" ) {
            return 1;
        }

        if( id == "u32" || id == "s32" || id == "string" ) {
            return 2;
        }

        return 3;
    }

    static bool isSigned( const std::string& id ) {
        return id == "s8" || id == "s16" || id == "s32";
    }

    static bool isOneSigned( const std::string& a, const std::string& b ) {
        return ( isSigned( a ) && !isSigned( b ) ) || ( !isSigned( a ) && isSigned( b ) );
    }

    static std::string scrubSigned( const std::string& id ) {
        if( id == "s8" ) { return "u8"; }
        if( id == "s16" ) { return "u16"; }
        if( id == "s32" ) { return "u32"; }

        return "<undefined>";
    }

    TokenType typeIdToTokenType( const std::string& id ) {
        if( id == "u8" ) { return TokenType::TOKEN_U8; }
        if( id == "u16" ) { return TokenType::TOKEN_U16; }
        if( id == "u32" ) { return TokenType::TOKEN_U32; }
        if( id == "s8" ) { return TokenType::TOKEN_S8; }
        if( id == "s16" ) { return TokenType::TOKEN_S16; }
        if( id == "s32" ) { return TokenType::TOKEN_S32; }
        if( id == "string" ) { return TokenType::TOKEN_STRING; }

        return TokenType::TOKEN_NONE;
    }

    std::string tokenTypeToTypeId( const TokenType type ) {
        switch( type ) {
            default:
                return "";
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

    static bool typeIsUdt( const std::string& typeId ) {
        return typeIdToTokenType( typeId ) == TokenType::TOKEN_NONE;
    }

    static std::optional< std::string > getIdentifierName( const Expression& node ) {
        if( auto primaryResult = std::get_if< std::unique_ptr< Primary > >( &node.value ) ) {
            const Primary& primary = **primaryResult;
            if( auto tokenResult = std::get_if< Token >( &primary.value ) ) {
                const Token& token = *tokenResult;
                if( token.type == TokenType::TOKEN_IDENTIFIER && token.value ) {
                    if( auto stringResult = std::get_if< std::string >( &*token.value ) ) {
                        return *stringResult;
                    }
                }
            }
        }

        return {};
    }

    std::string getType( const Primary& node, MemoryTracker& memory ) {
        return std::visit(
            overloaded {
                [ &memory ]( const Token& token ){
                    switch( token.type ) {
                        case TokenType::TOKEN_LITERAL_INTEGER: {
                            return getLiteralType( expectLong( token ) );
                        }
                        case TokenType::TOKEN_LITERAL_STRING: {
                            return std::string( "string" );
                        }
                        case TokenType::TOKEN_IDENTIFIER: {
                            std::string id = expectString( token );
                            auto memoryQuery = memory.find( id );
                            if( !memoryQuery ) {
                                return std::string( "<undefined>" );
                            }

                            std::string typeId = MemoryTracker::unwrapValue( *memoryQuery ).typeId;
                            if( typeIsUdt( typeId ) ) {
                                // Udt
                                std::optional< UserDefinedType > udt = memory.findUdt( typeId );
                                if( udt ) {
                                    return udt->id;
                                }

                                // If we dereference an identifier, but it doesn't have a valid type
                                // then something terrible happened that wasn't supposed to.
                                Error{ "Internal compiler error", token }.throwException();
                            }

                            return typeId;
                        }
                        default: {
                            Error{ "Internal compiler error", token }.throwException();
                            return std::string( "" );
                        }
                    }
                },
                [ &memory ]( const std::unique_ptr< Expression >& expression ){
                    return getType( *expression, memory );
                }
            },
            node.value
        );
    }

    std::string getType( const BinaryExpression& node, MemoryTracker& memory ) {

        std::string lhs = getType( *node.lhsValue, memory );
        std::string rhs = getType( *node.rhsValue, memory );

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
                    return "<undefined>";
                }

                auto lhsUdt = memory.findUdt( lhs );
                if( !lhsUdt ) {
                    return "<undefined>";
                }

                auto rhsUdtField = memory.findUdtField( lhsUdt->id, *rhsIdentifier );
                if( !rhsUdtField ) {
                    return "<undefined>";
                }

                return rhsUdtField->typeId;
            }
            default: {
                // For all other operators, if left hand side is a UDT, then RHS must be as well.
                if( typeIsUdt( lhs ) ) {
                    if( lhs == rhs ) {
                        return lhs;
                    } else {
                        return "<undefined>";
                    }
                }

                // If either side is undefined then we must return undefined
                if( lhs == "<undefined>" || rhs == "<undefined>" ) {
                    return "<undefined>";
                }

                // Otherwise the data type of the BinaryExpression is the larger of lhs, rhs
                if( getTypeComparison( rhs ) >= getTypeComparison( lhs ) ) {
                    return isOneSigned( lhs, rhs ) ? scrubSigned( rhs ) : rhs;
                } else {
                    return isOneSigned( lhs, rhs ) ? scrubSigned( lhs ) : lhs;
                }
            }
        }
    }

    std::string getType( const Expression& node, MemoryTracker& memory ) {
		if( auto binaryExpression = std::get_if< std::unique_ptr< BinaryExpression > >( &node.value ) ) {
 			return getType( **binaryExpression, memory );
		}

		if( auto primaryExpression = std::get_if< std::unique_ptr< Primary > >( &node.value ) ) {
			return getType( **primaryExpression, memory );
		}

		// Many node types not yet implemented
        return "";
    }

}
