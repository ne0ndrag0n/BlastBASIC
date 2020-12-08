#include "generator.hpp"
#include "variant_visitor.hpp"
#include "memory_tracker.hpp"
#include "token.hpp"
#include <cstdio>
#include <exception>

namespace GoldScorpion {

	static MemoryTracker memory;

	static char getLiteralType( long literal ) {
		// Negative values mean a signed value is required
		if( literal < 0 ) {

			if( literal >= -127 ) {
				return 'b';
			}

			if( literal >= -32767 ) {
				return 'w';
			}

			return 'l';

		} else {
			
			if( literal <= 255 ) {
				return 'b';
			}

			if( literal <= 65535 ) {
				return 'w';
			}

			return 'l';
		}
	}

	static long expectLong( const Token& token, const std::string& error ) {
		if( token.value ) {
			if( auto longValue = std::get_if< long >( &*( token.value ) ) ) {
				return *longValue;
			}
		}

		throw std::runtime_error( error );
	}

	// Child is a literal or a single identifier - use directly
	// OTherwise, generate that node, and use its value off the stack
	// BinaryExpressions always evaluated left to right
	static void generate( const BinaryExpression& node, Assembly& assembly ) {
		bool stackLeft = false;
		char buffer[ 50 ] = { 0 };

		if( auto primary = std::get_if< std::unique_ptr< Primary > >( &( node.lhsValue->value ) ) ) {
			if( auto token = std::get_if< Token >( &((*primary)->value) ) ) {
				// Verify token is either: identifier or numeric literal
				if( token->type == TokenType::TOKEN_LITERAL_INTEGER ) {
					long value = expectLong( *token, "Expected: literal integer on LHS of BinaryExpression" );

					std::sprintf( buffer, "\tmove.%c #%ld, d0\n", getLiteralType( value ), value );
					assembly.body += std::string( buffer );
					
				} else if( token->type == TokenType::TOKEN_IDENTIFIER ) {

				} else {

				}
			}
		}
	}

	Result< Assembly > generate( const Program& program ) {
		return "Not implemented";
	}

}