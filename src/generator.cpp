#include "generator.hpp"
#include "variant_visitor.hpp"
#include "memory_tracker.hpp"
#include "token.hpp"
#include <cstdio>
#include <exception>
#include <unordered_map>

namespace GoldScorpion {

	enum class ExpressionDataType { INVALID, U8, U16, U32, S8, S16, S32, STRING };

	static const std::unordered_map< std::string, ExpressionDataType > types = {
		{ "u8", ExpressionDataType::U8 },
		{ "u16", ExpressionDataType::U16 },
		{ "u32", ExpressionDataType::U32 },
		{ "s8", ExpressionDataType::S8 },
		{ "s16", ExpressionDataType::S16 },
		{ "s32", ExpressionDataType::S32 },
		{ "string", ExpressionDataType::STRING }
	};

	static MemoryTracker memory;

	static ExpressionDataType getLiteralType( long literal ) {
		// Negative values mean a signed value is required
		if( literal < 0 ) {

			if( literal >= -127 ) {
				return ExpressionDataType::S8;
			}

			if( literal >= -32767 ) {
				return ExpressionDataType::S16;
			}

			return ExpressionDataType::S32;

		} else {
			
			if( literal <= 255 ) {
				return ExpressionDataType::U8;
			}

			if( literal <= 65535 ) {
				return ExpressionDataType::U16;
			}

			return ExpressionDataType::U32;
		}
	}

	static ExpressionDataType getIdentifierType( const std::string& typeId ) {
		auto it = types.find( typeId );
		if( it != types.end() ) {
			return it->second;
		}

		return ExpressionDataType::INVALID;
	}

	static long expectLong( const Token& token, const std::string& error ) {
		if( token.value ) {
			if( auto longValue = std::get_if< long >( &*( token.value ) ) ) {
				return *longValue;
			}
		}

		throw std::runtime_error( error );
	}

	static std::string expectString( const Token& token, const std::string& error ) {
		if( token.value ) {
			if( auto stringValue = std::get_if< std::string >( &*( token.value ) ) ) {
				return *stringValue;
			}
		}

		throw std::runtime_error( error );
	}

	static ExpressionDataType getType( const Expression& node ) {
		// Not yet implemented
		return ExpressionDataType::INVALID;
	}

	static ExpressionDataType getType( const Primary& node ) {
		ExpressionDataType result;

		// We can directly determine the primary value for any Token variant
		// Otherwise, we need to go deeper
		std::visit( overloaded {
			[ &result ]( const Token& token ) {
				// The only two types of token expected here are TOKEN_LITERAL_INTEGER, TOKEN_LITERAL_STRING, and TOKEN_IDENTIFIER
				// If TOKEN_IDENTIFIER is provided, the underlying type must not be of a custom type
				switch( token.type ) {
					case TokenType::TOKEN_LITERAL_INTEGER: {
						result = getLiteralType( expectLong( token, "Expected: long type for literal integer token" ) );
						break;
					}
					case TokenType::TOKEN_LITERAL_STRING: {
						result = ExpressionDataType::STRING;
						break;
					}
					case TokenType::TOKEN_IDENTIFIER: {
						std::string id = expectString( token, "Expected: string type for identifier token" );
						auto memoryQuery = memory.find( id );
						if( !memoryQuery ) {
							throw std::runtime_error( std::string( "Undefined identifier: " ) + id );
						}

						result = getIdentifierType( std::visit( overloaded {
							[]( const GlobalMemoryElement& element ) { return element.value.typeId; },
							[]( const StackMemoryElement& element ) { return element.value.typeId; }
						}, *memoryQuery ) );
						break;
					}
					default:
						throw std::runtime_error( "Expected: integer, string, or identifier as expression operand" );
				}
			},
			[ &result ]( const std::unique_ptr< Expression >& expression ) {
				result = getType( *expression );
			}
		}, node.value );

		return result;
	}

	static ExpressionDataType getType( const BinaryExpression& node ) {
		// The type of a BinaryExpression is the larger of the two children

		ExpressionDataType lhs;
		ExpressionDataType rhs;

		if( auto primaryValue = std::get_if< std::unique_ptr< Primary > >( &node.lhsValue->value ) ) {
			lhs = getType( **primaryValue );
		} else {
			// There is a subexpression that needs to be evaluated
			lhs = getType( *node.lhsValue );
		}

		return ExpressionDataType::INVALID;
	}

	// Child is a literal or a single identifier - use directly
	// OTherwise, generate that node, and use its value off the stack
	// BinaryExpressions always evaluated left to right
	static void generate( const BinaryExpression& node, Assembly& assembly ) {
		bool stackLeft = false;
		char buffer[ 50 ] = { 0 };

		// Get type of left and right hand sides
		// The largest of the two types is used to generate code
	}

	Result< Assembly > generate( const Program& program ) {
		return "Not implemented";
	}

}