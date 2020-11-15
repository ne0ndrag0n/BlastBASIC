#include "lexer.hpp"
#include "variant_visitor.hpp"
#include <unordered_map>

namespace GoldScorpion {

	static const std::unordered_map< std::string, TokenType > TOKEN_MAP = {
		{ "def", TokenType::TOKEN_DEF },
		{ "as", TokenType::TOKEN_AS },
		{ "u8", TokenType::TOKEN_U8 },
		{ "u16", TokenType::TOKEN_U16 },
		{ "u32", TokenType::TOKEN_U32 },
		{ "s8", TokenType::TOKEN_S8 },
		{ "s16", TokenType::TOKEN_S16 },
		{ "s32", TokenType::TOKEN_S32 },
		{ "string", TokenType::TOKEN_STRING },
		{ "+", TokenType::TOKEN_PLUS },
		{ "-", TokenType::TOKEN_MINUS },
		{ "*", TokenType::TOKEN_ASTERISK },
		{ "/", TokenType::TOKEN_FORWARD_SLASH },
		{ ".", TokenType::TOKEN_DOT },
		{ "(", TokenType::TOKEN_LEFT_PAREN },
		{ ")", TokenType::TOKEN_RIGHT_PAREN },
		{ "=", TokenType::TOKEN_EQUALS },
		{ "==", TokenType::TOKEN_DOUBLE_EQUALS },
		{ "not", TokenType::TOKEN_NOT },
		{ "then", TokenType::TOKEN_THEN },
		{ "function", TokenType::TOKEN_FUNCTION },
		{ ",", TokenType::TOKEN_COMMA },
		{ "end", TokenType::TOKEN_END },
		{ "type", TokenType::TOKEN_TYPE },
		{ "return", TokenType::TOKEN_RETURN },
		{ "import", TokenType::TOKEN_IMPORT },
		{ "[", TokenType::TOKEN_LEFT_BRACKET },
		{ "]", TokenType::TOKEN_RIGHT_BRACKET },
		{ "asm", TokenType::TOKEN_ASM },
		{ "this", TokenType::TOKEN_THIS },
		{ ">", TokenType::TOKEN_GREATER_THAN },
		{ "<", TokenType::TOKEN_LESS_THAN },
		{ ">=", TokenType::TOKEN_GREATER_THAN_EQUAL },
		{ "<=", TokenType::TOKEN_LESS_THAN_EQUAL },
		{ ">>", TokenType::TOKEN_SHIFT_RIGHT },
		{ "<<", TokenType::TOKEN_SHIFT_LEFT },
		{ "byref", TokenType::TOKEN_BYREF },
		{ "\"", TokenType::TOKEN_DOUBLE_QUOTE }
	};

	static Result< Token > interpretToken( std::string& segment, bool& numericComponent ) {
		if( numericComponent ) {
			// Numeric component state can be parsed straight
			Token result = Token{ TokenType::TOKEN_LITERAL_INTEGER, std::stol( segment ) };

			// Reset lexer state
			segment = "";
			numericComponent = false;

			return result;
		}

		Token result;

		// Otherwise we must verify token is one of the following multipart tokens


		// Reset lexer state
		segment = "";
		numericComponent = false;

		return result;
	}

	static bool isNumeric( char c ) {
		return c >= '0' && c <= '9';
	}

	static bool isAlpha( char c ) {
		return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
	}

	Result< std::queue< Token > > getTokens( const std::string& body ) {
		struct Line {
			unsigned int line = 1;
			unsigned int column = 1;
		};

		std::queue< Token > tokens;

		std::string component;
		bool lineContinuation = false;
		bool numericComponent = false;
		for( const char& letter : body ) {
			switch( letter ) {
				case '\n': {
					if( lineContinuation ) {
						// If line continuation operator is present
						// Then eat the newline instead of adding it to the token stream
						lineContinuation = false;
						continue;
					}

					[[fallthrough]];
				}
				case ' ':
				case '\t':
				case '\r':
				case '\v':
				case '\f': {
					// Interpret + push the current token
					if( component != "" ) {
						Result< Token > result = interpretToken( component, numericComponent );

						if( auto token = std::get_if< Token >( &result ) ) {
							tokens.push( *token );
						} else {
							// Error
							return std::get< std::string >( result );
						}
					}

					// Mark a newline if newline encountered
					if( letter == '\n' ) {
						tokens.push( Token{ TokenType::TOKEN_NEWLINE, {} } );
					}
					continue;
				}
				case '\\': {
					// When encountering the line-continuation operator, set the lineContinuation flag for when the next \n is encountered
					lineContinuation = true;
					continue;
				}
				default: {
					// Token that begins with number will ultimately be interpreted as a number if it is the first part of a new component
					if( isNumeric( letter ) ) {
						if( component == "" ) {
							numericComponent = true;
						}

						component += letter;
					} else if( isAlpha( letter ) ) {
						component += letter;
					} else {
						// Error
						return "Invalid character";
					}
				}
			}
		}

		return "Not implemented";
	}
}