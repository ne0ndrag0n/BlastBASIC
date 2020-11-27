#include "token.hpp"

namespace GoldScorpion {

	std::string Token::toString() const {
		std::string result = "Token(TokenType::";

		switch( type ) {
			case TokenType::TOKEN_NONE:
				return result + "TOKEN_NONE)";
			case TokenType::TOKEN_DEF:
				return result + "TOKEN_DEF)";
			case TokenType::TOKEN_IDENTIFIER:
				return result + "TOKEN_IDENTIFIER)";
			case TokenType::TOKEN_AS:
				return result + "TOKEN_AS)";
			case TokenType::TOKEN_U8:
				return result + "TOKEN_U8)";
			case TokenType::TOKEN_U16:
				return result + "TOKEN_U16)";
			case TokenType::TOKEN_U32:
				return result + "TOKEN_U32)";
			case TokenType::TOKEN_S8:
				return result + "TOKEN_S8)";
			case TokenType::TOKEN_S16:
				return result + "TOKEN_S16)";
			case TokenType::TOKEN_S32:
				return result + "TOKEN_S32)";
			case TokenType::TOKEN_STRING:
				return result + "TOKEN_STRING)";
			case TokenType::TOKEN_LITERAL_STRING:
				return result + "TOKEN_LITERAL_STRING " + std::get< std::string >( *value ) + ")";
			case TokenType::TOKEN_LITERAL_INTEGER:
				return result + "TOKEN_LITERAL_INTEGER " + std::to_string( std::get< long >( *value ) ) + ")";
			case TokenType::TOKEN_PLUS:
				return result + "TOKEN_PLUS)";
			case TokenType::TOKEN_MINUS:
				return result + "TOKEN_MINUS)";
			case TokenType::TOKEN_ASTERISK:
				return result + "TOKEN_ASTERISK)";
			case TokenType::TOKEN_FORWARD_SLASH:
				return result + "TOKEN_FORWARD_SLASH)";
			case TokenType::TOKEN_BACK_SLASH:
				return result + "TOKEN_BACK_SLASH)";
			case TokenType::TOKEN_DOT:
				return result + "TOKEN_DOT)";
			case TokenType::TOKEN_LEFT_PAREN:
				return result + "TOKEN_LEFT_PAREN)";
			case TokenType::TOKEN_RIGHT_PAREN:
				return result + "TOKEN_RIGHT_PAREN)";
			case TokenType::TOKEN_EQUALS:
				return result + "TOKEN_EQUALS)";
			case TokenType::TOKEN_NOT_EQUALS:
				return result + "TOKEN_NOT_EQUALS)";
			case TokenType::TOKEN_DOUBLE_EQUALS:
				return result + "TOKEN_DOUBLE_EQUALS)";
			case TokenType::TOKEN_NOT:
				return result + "TOKEN_NOT)";
			case TokenType::TOKEN_THEN:
				return result + "TOKEN_THEN)";
			case TokenType::TOKEN_FUNCTION:
				return result + "TOKEN_FUNCTION)";
			case TokenType::TOKEN_COMMA:
				return result + "TOKEN_COMMA)";
			case TokenType::TOKEN_END:
				return result + "TOKEN_END)";
			case TokenType::TOKEN_TYPE:
				return result + "TOKEN_TYPE)";
			case TokenType::TOKEN_RETURN:
				return result + "TOKEN_RETURN)";
			case TokenType::TOKEN_IMPORT:
				return result + "TOKEN_IMPORT)";
			case TokenType::TOKEN_LEFT_BRACKET:
				return result + "TOKEN_LEFT_BRACKET)";
			case TokenType::TOKEN_RIGHT_BRACKET:
				return result + "TOKEN_RIGHT_BRACKET)";
			case TokenType::TOKEN_ASM:
				return result + "TOKEN_ASM)";
			case TokenType::TOKEN_DOUBLE_FORWARD_SLASH:
				return result + "TOKEN_DOUBLE_FORWARD_SLASH)";
			case TokenType::TOKEN_THIS:
				return result + "TOKEN_THIS)";
			case TokenType::TOKEN_NEWLINE:
				return result + "TOKEN_NEWLINE)";
			case TokenType::TOKEN_GREATER_THAN:
				return result + "TOKEN_GREATER_THAN)";
			case TokenType::TOKEN_LESS_THAN:
				return result + "TOKEN_LESS_THAN)";
			case TokenType::TOKEN_GREATER_THAN_EQUAL:
				return result + "TOKEN_GREATER_THAN_EQUAL)";
			case TokenType::TOKEN_LESS_THAN_EQUAL:
				return result + "TOKEN_LESS_THAN_EQUAL)";
			case TokenType::TOKEN_SHIFT_RIGHT:
				return result + "TOKEN_SHIFT_RIGHT)";
			case TokenType::TOKEN_SHIFT_LEFT:
				return result + "TOKEN_SHIFT_LEFT)";
			case TokenType::TOKEN_BYREF:
				return result + "TOKEN_BYREF)";
			case TokenType::TOKEN_DOUBLE_QUOTE:
				return result + "TOKEN_DOUBLE_QUOTE)";
			case TokenType::TOKEN_IF:
				return result + "TOKEN_IF)";
			case TokenType::TOKEN_FOR:
				return result + "TOKEN_FOR)";
			case TokenType::TOKEN_WHILE:
				return result + "TOKEN_WHILE)";
			case TokenType::TOKEN_TO:
				return result + "TOKEN_TO)";
			case TokenType::TOKEN_EVERY:
				return result + "TOKEN_EVERY)";
			case TokenType::TOKEN_ELSE:
				return result + "TOKEN_ELSE)";
			case TokenType::TOKEN_BREAK:
				return result + "TOKEN_BREAK)";
			case TokenType::TOKEN_CONTINUE:
				return result + "TOKEN_CONTINUE)";
			case TokenType::TOKEN_AND:
				return result + "TOKEN_AND)";
			case TokenType::TOKEN_OR:
				return result + "TOKEN_OR)";
			case TokenType::TOKEN_XOR:
				return result + "TOKEN_XOR)";
			case TokenType::TOKEN_SUPER:
				return result + "TOKEN_SUPER)";
			case TokenType::TOKEN_MODULO:
				return result + "TOKEN_MODULO)";
			default:
				return result + "<unknown>)";
		}
	}

}