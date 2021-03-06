#pragma once
#include <optional>
#include <variant>
#include <string>

namespace GoldScorpion {

	enum class TokenType {
		TOKEN_NONE,
		TOKEN_DEF,
		TOKEN_IDENTIFIER,
		TOKEN_AS,
		TOKEN_U8,
		TOKEN_U16,
		TOKEN_U32,
		TOKEN_S8,
		TOKEN_S16,
		TOKEN_S32,
		TOKEN_STRING,
		TOKEN_LITERAL_STRING,
		TOKEN_LITERAL_INTEGER,
		TOKEN_PLUS,
		TOKEN_MINUS,
		TOKEN_ASTERISK,
		TOKEN_FORWARD_SLASH,
		TOKEN_BACK_SLASH,
		TOKEN_DOT,
		TOKEN_LEFT_PAREN,
		TOKEN_RIGHT_PAREN,
		TOKEN_EQUALS,
		TOKEN_NOT_EQUALS,
		TOKEN_DOUBLE_EQUALS,
		TOKEN_NOT,
		TOKEN_THEN,
		TOKEN_FUNCTION,
		TOKEN_COMMA,
		TOKEN_END,
		TOKEN_TYPE,
		TOKEN_RETURN,
		TOKEN_IMPORT,
		TOKEN_LEFT_BRACKET,
		TOKEN_RIGHT_BRACKET,
		TOKEN_ASM,
		TOKEN_DOUBLE_FORWARD_SLASH,
		TOKEN_THIS,
		TOKEN_NEWLINE,
		TOKEN_GREATER_THAN,
		TOKEN_LESS_THAN,
		TOKEN_GREATER_THAN_EQUAL,
		TOKEN_LESS_THAN_EQUAL,
		TOKEN_SHIFT_RIGHT,
		TOKEN_SHIFT_LEFT,
		TOKEN_BYREF,
		TOKEN_DOUBLE_QUOTE,
		TOKEN_IF,
		TOKEN_FOR,
		TOKEN_WHILE,
		TOKEN_TO,
		TOKEN_EVERY,
		TOKEN_ELSE,
		TOKEN_BREAK,
		TOKEN_CONTINUE,
		TOKEN_AND,
		TOKEN_OR,
		TOKEN_XOR,
		TOKEN_SUPER,
		TOKEN_MODULO,
		TOKEN_AMPERSAND,
		TOKEN_CARET,
		TOKEN_PIPE,
		TOKEN_TEXT,
		TOKEN_AT_SYMBOL,
		TOKEN_CONST
	};

	struct Token {
		TokenType type;
		std::optional< std::variant< long, std::string > > value;
		unsigned int line;
		unsigned int column;

		std::string toString() const;
	};

}
