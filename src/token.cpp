#include "token.hpp"

namespace GoldScorpion {

	std::string Token::toString() const {
		std::string result = "Token(TokenType::";

		std::string occurrence = " " + std::to_string( line ) + ", " + std::to_string( column );

		switch( type ) {
			case TokenType::TOKEN_NONE:
				return result + "TOKEN_NONE)" + occurrence;
			case TokenType::TOKEN_DEF:
				return result + "TOKEN_DEF)" + occurrence;
			case TokenType::TOKEN_IDENTIFIER:
				return result + "TOKEN_IDENTIFIER)" + occurrence;
			case TokenType::TOKEN_AS:
				return result + "TOKEN_AS)" + occurrence;
			case TokenType::TOKEN_U8:
				return result + "TOKEN_U8)" + occurrence;
			case TokenType::TOKEN_U16:
				return result + "TOKEN_U16)" + occurrence;
			case TokenType::TOKEN_U32:
				return result + "TOKEN_U32)" + occurrence;
			case TokenType::TOKEN_S8:
				return result + "TOKEN_S8)" + occurrence;
			case TokenType::TOKEN_S16:
				return result + "TOKEN_S16)" + occurrence;
			case TokenType::TOKEN_S32:
				return result + "TOKEN_S32)" + occurrence;
			case TokenType::TOKEN_STRING:
				return result + "TOKEN_STRING)" + occurrence;
			case TokenType::TOKEN_LITERAL_STRING:
				return result + "TOKEN_LITERAL_STRING " + std::get< std::string >( *value ) + ")" + occurrence;
			case TokenType::TOKEN_LITERAL_INTEGER:
				return result + "TOKEN_LITERAL_INTEGER " + std::to_string( std::get< long >( *value ) ) + ")" + occurrence;
			case TokenType::TOKEN_PLUS:
				return result + "TOKEN_PLUS)" + occurrence;
			case TokenType::TOKEN_MINUS:
				return result + "TOKEN_MINUS)" + occurrence;
			case TokenType::TOKEN_ASTERISK:
				return result + "TOKEN_ASTERISK)" + occurrence;
			case TokenType::TOKEN_FORWARD_SLASH:
				return result + "TOKEN_FORWARD_SLASH)" + occurrence;
			case TokenType::TOKEN_BACK_SLASH:
				return result + "TOKEN_BACK_SLASH)" + occurrence;
			case TokenType::TOKEN_DOT:
				return result + "TOKEN_DOT)" + occurrence;
			case TokenType::TOKEN_LEFT_PAREN:
				return result + "TOKEN_LEFT_PAREN)" + occurrence;
			case TokenType::TOKEN_RIGHT_PAREN:
				return result + "TOKEN_RIGHT_PAREN)" + occurrence;
			case TokenType::TOKEN_EQUALS:
				return result + "TOKEN_EQUALS)" + occurrence;
			case TokenType::TOKEN_NOT_EQUALS:
				return result + "TOKEN_NOT_EQUALS)" + occurrence;
			case TokenType::TOKEN_DOUBLE_EQUALS:
				return result + "TOKEN_DOUBLE_EQUALS)" + occurrence;
			case TokenType::TOKEN_NOT:
				return result + "TOKEN_NOT)" + occurrence;
			case TokenType::TOKEN_THEN:
				return result + "TOKEN_THEN)" + occurrence;
			case TokenType::TOKEN_FUNCTION:
				return result + "TOKEN_FUNCTION)" + occurrence;
			case TokenType::TOKEN_COMMA:
				return result + "TOKEN_COMMA)" + occurrence;
			case TokenType::TOKEN_END:
				return result + "TOKEN_END)" + occurrence;
			case TokenType::TOKEN_TYPE:
				return result + "TOKEN_TYPE)" + occurrence;
			case TokenType::TOKEN_RETURN:
				return result + "TOKEN_RETURN)" + occurrence;
			case TokenType::TOKEN_IMPORT:
				return result + "TOKEN_IMPORT)" + occurrence;
			case TokenType::TOKEN_LEFT_BRACKET:
				return result + "TOKEN_LEFT_BRACKET)" + occurrence;
			case TokenType::TOKEN_RIGHT_BRACKET:
				return result + "TOKEN_RIGHT_BRACKET)" + occurrence;
			case TokenType::TOKEN_ASM:
				return result + "TOKEN_ASM)" + occurrence;
			case TokenType::TOKEN_DOUBLE_FORWARD_SLASH:
				return result + "TOKEN_DOUBLE_FORWARD_SLASH)" + occurrence;
			case TokenType::TOKEN_THIS:
				return result + "TOKEN_THIS)" + occurrence;
			case TokenType::TOKEN_NEWLINE:
				return result + "TOKEN_NEWLINE)" + occurrence;
			case TokenType::TOKEN_GREATER_THAN:
				return result + "TOKEN_GREATER_THAN)" + occurrence;
			case TokenType::TOKEN_LESS_THAN:
				return result + "TOKEN_LESS_THAN)" + occurrence;
			case TokenType::TOKEN_GREATER_THAN_EQUAL:
				return result + "TOKEN_GREATER_THAN_EQUAL)" + occurrence;
			case TokenType::TOKEN_LESS_THAN_EQUAL:
				return result + "TOKEN_LESS_THAN_EQUAL)" + occurrence;
			case TokenType::TOKEN_SHIFT_RIGHT:
				return result + "TOKEN_SHIFT_RIGHT)" + occurrence;
			case TokenType::TOKEN_SHIFT_LEFT:
				return result + "TOKEN_SHIFT_LEFT)" + occurrence;
			case TokenType::TOKEN_BYREF:
				return result + "TOKEN_BYREF)" + occurrence;
			case TokenType::TOKEN_DOUBLE_QUOTE:
				return result + "TOKEN_DOUBLE_QUOTE)" + occurrence;
			case TokenType::TOKEN_IF:
				return result + "TOKEN_IF)" + occurrence;
			case TokenType::TOKEN_FOR:
				return result + "TOKEN_FOR)" + occurrence;
			case TokenType::TOKEN_WHILE:
				return result + "TOKEN_WHILE)" + occurrence;
			case TokenType::TOKEN_TO:
				return result + "TOKEN_TO)" + occurrence;
			case TokenType::TOKEN_EVERY:
				return result + "TOKEN_EVERY)" + occurrence;
			case TokenType::TOKEN_ELSE:
				return result + "TOKEN_ELSE)" + occurrence;
			case TokenType::TOKEN_BREAK:
				return result + "TOKEN_BREAK)" + occurrence;
			case TokenType::TOKEN_CONTINUE:
				return result + "TOKEN_CONTINUE)" + occurrence;
			case TokenType::TOKEN_AND:
				return result + "TOKEN_AND)" + occurrence;
			case TokenType::TOKEN_OR:
				return result + "TOKEN_OR)" + occurrence;
			case TokenType::TOKEN_XOR:
				return result + "TOKEN_XOR)" + occurrence;
			case TokenType::TOKEN_SUPER:
				return result + "TOKEN_SUPER)" + occurrence;
			case TokenType::TOKEN_MODULO:
				return result + "TOKEN_MODULO)" + occurrence;
			case TokenType::TOKEN_AMPERSAND:
				return result + "TOKEN_AMPERSAND)" + occurrence;
			case TokenType::TOKEN_CARET:
				return result + "TOKEN_CARET)" + occurrence;
			case TokenType::TOKEN_PIPE:
				return result + "TOKEN_PIPE)" + occurrence;
			case TokenType::TOKEN_TEXT:
				return result + "TOKEN_TEXT " + std::get< std::string >( *value ) + ")" + occurrence;
			case TokenType::TOKEN_AT_SYMBOL:
				return result + "TOKEN_AT_SYMBOL)" + occurrence;
			case TokenType::TOKEN_CONST:
				return result + "TOKEN_CONST)" + occurrence;
			default:
				return result + "<unknown>)" + occurrence;
		}
	}

}